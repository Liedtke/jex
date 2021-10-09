#include <jex_codegenvisitor.hpp>

#include <jex_ast.hpp>
#include <jex_codegenutils.hpp>
#include <jex_codemodule.hpp>
#include <jex_constantstore.hpp>
#include <jex_compileenv.hpp>
#include <jex_errorhandling.hpp>
#include <jex_fctinfo.hpp>
#include <jex_fctlibrary.hpp>
#include <jex_symboltable.hpp>
#include <jex_unwind.hpp>

#include "llvm/Support/FormatVariadic.h"

#include <set>
#include <sstream>

namespace jex {

static llvm::StringRef toLlvm(std::string_view str) {
    return llvm::StringRef(str.data(), str.length());
}

CodeGenVisitor::CodeGenVisitor(CompileEnv& env)
: d_env(env) {
}

CodeGenVisitor::~CodeGenVisitor() {
}

llvm::BasicBlock* CodeGenVisitor::createBlock(const char* name) {
    assert(d_currFct);
    // Insert regular blocks in front of unwinding blocks for better readability.
    llvm::BasicBlock* insertPoint = d_unwind ? d_unwind->getEntryBlock() : nullptr;
    return llvm::BasicBlock::Create(d_module->llvmContext(), name, d_currFct, insertPoint);
}

void CodeGenVisitor::createInit(const Symbol* sym) {
    llvm::Value* var = getVarPtr(sym);
    TypeInfoId type = sym->type;
    if (type->isZeroInitialized()) {
        // Simplified initialization instead of function call.
        d_builder->CreateStore(llvm::Constant::getNullValue(var->getType()->getPointerElementType()), var);
        return;
    }
    // Call constructor.
    const FctInfo& ctor = d_env.fctLibrary().getFct("_ctor_" + type->name(), {});
    assert(ctor.d_retType == type && "constructor has invalid return type");
    llvm::FunctionCallee ctorCallee = d_utils->getOrCreateFct(&ctor);
    if (type->callConv() == TypeInfo::CallConv::ByValue) {
        var = d_builder->CreateLoad(var);
    }
    d_builder->CreateCall(ctorCallee, {var});
}

void CodeGenVisitor::createDestruct(const Symbol* sym) {
    TypeInfoId type = sym->type;
    if (type->kind() == TypeKind::Value) {
        return; // Nothing to do for value types.
    }
    assert(type->kind() == TypeKind::Complex &&
           "The context may only contain value and complex types");
    // Call destructor.
    const FctInfo& dtor = d_env.fctLibrary().getFct("_dtor_" + type->name(), {});
    assert(dtor.d_retType == type && "destructor has invalid return type");
    llvm::FunctionCallee dtorCallee = d_utils->getOrCreateFct(&dtor);
    d_builder->CreateCall(dtorCallee, {getVarPtr(sym)});
}

template<typename Iter>
void CodeGenVisitor::createInitDestructFct(Iter symBegin, Iter symEnd, const char* prefix,
                                           void(CodeGenVisitor::*createCall)(const Symbol*)) {
    // Create Function.
    llvm::Type* voidTy = llvm::Type::getVoidTy(d_module->llvmContext());
    llvm::Type* rctxPtrTy = llvm::StructType::getTypeByName(d_module->llvmContext(), "Rctx")->getPointerTo();
    llvm::FunctionType* fctType = llvm::FunctionType::get(voidTy, {rctxPtrTy}, false);
    d_currFct = llvm::Function::Create(
        fctType, llvm::GlobalValue::LinkageTypes::ExternalLinkage, llvm::Twine(prefix) + "_rctx", d_module->llvmModule());
    d_currFct->getArg(0)->setName("rctx");
    d_builder->SetInsertPoint(createBlock("entry"));
    // Initialize all variables in context.
    Iter iter = symBegin;
    while (iter != symEnd) {
        const Symbol* sym = *iter;
        (this->*createCall)(sym);
        ++iter;
    }
    d_builder->CreateRetVoid();
}

void CodeGenVisitor::createIR() {
    d_offsets.clear();
    d_module = std::make_unique<CodeModule>(d_env);
    d_utils = std::make_unique<CodeGenUtils>(d_env, *d_module);
    d_builder = std::make_unique<llvm::IRBuilder<>>(d_module->llvmContext());
    // Order by alignment descending, then by symbol name ascending.
    auto cmp = [](const Symbol* a, const Symbol* b) {
        return a->type->alignment() > b->type->alignment() ? true : a->name < b->name;
    };
    std::set<const Symbol*, decltype(cmp)> vars(cmp);
    for (AstVariableDef* varDef: d_env.getRoot()->d_varDefs) {
        vars.insert(varDef->d_name->d_symbol);
    }
    size_t offset = 0;
    for (const Symbol* sym : vars) {
        d_offsets.emplace(sym, offset);
        offset += sym->type->size();
    }
    d_env.setContextSize(offset);
    d_rctxType = llvm::StructType::create(d_module->llvmContext(), "Rctx");
    d_env.getRoot()->accept(*this);
    // Generate lifetime functions for context.
    createInitDestructFct(vars.begin(), vars.end(), "__init", &CodeGenVisitor::createInit);
    createInitDestructFct(vars.begin(), vars.end(), "__destruct", &CodeGenVisitor::createDestruct);
}

llvm::Value* CodeGenVisitor::visitExpression(IAstExpression& node) {
    assert(d_result == nullptr);
    node.accept(*this);
    assert(d_result != nullptr);
    return std::exchange(d_result, nullptr);
}

llvm::Value* CodeGenVisitor::getVarPtr(const Symbol* varSym) {
    // Get rctx as i8*.
    llvm::Value* rctx = d_currFct->getArg(0);
    llvm::Type* bytePtrTy = llvm::Type::getInt8PtrTy(d_module->llvmContext());
    llvm::Value* rctxAsI8Ptr = d_builder->CreatePointerCast(rctx, bytePtrTy, "rctxAsBytePtr");
    // Apply offset to rctx pointer.
    llvm::Value* offset = llvm::ConstantInt::get(d_module->llvmContext(), llvm::APInt(64, d_offsets[varSym]));
    llvm::Value* varPtr = d_builder->CreateGEP(bytePtrTy->getPointerElementType(), rctxAsI8Ptr, offset, "varPtr");
    // Reinterpret cast to target type.
    return d_builder->CreatePointerCast(varPtr, d_utils->getType(varSym->type)->getPointerTo(), "varPtrTyped");
}

void CodeGenVisitor::createAssign(llvm::Value* result, llvm::Value* source, TypeInfoId type) {
    assert(result->getType() == source->getType() && "Assign expects two pointers of the same type");
    const FctInfo& assign = d_env.fctLibrary().getFct("_assign", {type});
    assert(assign.d_retType == type && "Return type of assign has to be equal to its parameter type");
    llvm::FunctionCallee assignCallee = d_utils->getOrCreateFct(&assign);
    d_builder->CreateCall(assignCallee, {result, source});
}

void CodeGenVisitor::visit(AstVariableDef& node) {
    assert(d_currFct == nullptr);
    assert(!d_unwind);
    // Create function.
    llvm::Type* resultPtrType = d_utils->getReturnType(node.d_type->d_resultType);
    llvm::FunctionType* fctType = llvm::FunctionType::get(resultPtrType, {d_rctxType->getPointerTo()}, false);
    d_currFct = llvm::Function::Create(
        fctType, llvm::GlobalValue::LinkageTypes::ExternalLinkage, toLlvm(node.d_name->d_name), d_module->llvmModule());
    d_currFct->getArg(0)->setName("rctx");
    // Initialize unwinding for handling lifetime.
    d_unwind = std::make_unique<Unwind>(d_env, *d_module, *d_utils, d_currFct);
    // Create "basic function structure".
    llvm::BasicBlock* allocaBlock = createBlock("entry");
    llvm::BasicBlock* blockBegin = createBlock("begin");
    d_builder->SetInsertPoint(blockBegin);
    // Evaluate expression and store result.
    llvm::Value* result = visitExpression(*node.d_expr);
    // FIXME: Figure out how to handle different calling conventions.
    llvm::Value* varPtr = getVarPtr(node.d_name->d_symbol);
    if (node.d_resultType->kind() == TypeKind::Complex) {
        // TODO: Figure out if result is temporary and generate move assign instead.
        createAssign(varPtr, result, node.d_resultType);
    } else {
        // Perform a simple store.
        if (node.d_expr->d_resultType->callConv() == TypeInfo::CallConv::ByPointer) {
            // Load value to copy struct later on into varPtr.
            result = d_builder->CreateLoad(result);
        }
        d_builder->CreateStore(result, varPtr);
    }
    d_unwind->finalize(d_builder->GetInsertBlock(), varPtr);
    // Link allocas block to begin block.
    d_builder->SetInsertPoint(allocaBlock);
    d_builder->CreateBr(blockBegin);
    d_unwind.reset();
    d_currFct = nullptr;
}

void CodeGenVisitor::visit(AstLiteralExpr& node) {
    d_result = std::visit(overloaded {
        [&](int64_t val) -> llvm::Value* {
            return llvm::ConstantInt::get(d_module->llvmContext(), llvm::APInt(64, val));
        },
        [&](double val) -> llvm::Value* {
            return llvm::ConstantFP::get(d_module->llvmContext(), llvm::APFloat(val));
        },
        [&](bool val) -> llvm::Value* {
            return llvm::ConstantInt::get(d_module->llvmContext(), llvm::APInt(1, val));
        },
        [&](std::string_view val) -> llvm::Value* {
            TypeInfoId strType = d_env.typeSystem().getType("String");
            std::string constantName = llvm::formatv("strLit_l{0}_c{1}", node.d_loc.begin.line, node.d_loc.begin.col);
            const FctInfo& dtor = d_env.fctLibrary().getFct("_dtor_" + strType->name(), {});
            const std::string* constStr = d_env.constants().emplace<std::string>(constantName, dtor, val);
            auto linkage = llvm::GlobalValue::LinkageTypes::ExternalLinkage;
            llvm::Value* var = new llvm::GlobalVariable(d_module->llvmModule(), d_utils->getType(strType),
                /*isConstant*/true, linkage, nullptr, constantName);
            (void)constStr; // FIXME: Store mapping from name to constant pointer somewhere to link later on.
            return var;
        }
    }, node.d_value);
}

void CodeGenVisitor::visit(AstBinaryExpr& node) {
    // Generate argument evaluation.
    llvm::Value* lhs = visitExpression(*node.d_lhs);
    llvm::Value* rhs = visitExpression(*node.d_rhs);
    // Generate alloca to store the result.
    llvm::Type* resType = d_utils->getType(node.d_resultType);
    d_result = new llvm::AllocaInst(resType, 0, "res_" + node.d_fctInfo->d_name, &d_currFct->getEntryBlock());
    llvm::FunctionCallee fct = d_utils->getOrCreateFct(node.d_fctInfo);
    // Call the function.
    d_builder->CreateCall(fct.getFunctionType(), fct.getCallee(), {d_result, lhs, rhs});
    if (node.d_resultType->callConv() == TypeInfo::CallConv::ByValue) {
        d_result = d_builder->CreateLoad(d_result);
    }
    d_unwind->add(node, d_result);
}

void CodeGenVisitor::visit(AstFctCall& node) {
    // Visit arguments.
    std::vector<llvm::Value*> args({nullptr}); // First arg will be result alloca.
    for (IAstExpression* expr : node.d_args->d_args) {
        args.push_back(visitExpression(*expr));
    }
    // Generate alloca to store the result (after argument visit for better code readability).
    llvm::Type* resType = d_utils->getType(node.d_resultType);
    llvm::Value* res = new llvm::AllocaInst(resType, 0, "res_" + node.d_fctInfo->d_name, &d_currFct->getEntryBlock());
    args[0] = res;
    llvm::FunctionCallee fct = d_utils->getOrCreateFct(node.d_fctInfo);
    // Call the function.
    d_builder->CreateCall(fct.getFunctionType(), fct.getCallee(), args);
    d_result = res;
    if (node.d_resultType->callConv() == TypeInfo::CallConv::ByValue) {
        d_result = d_builder->CreateLoad(d_result);
    }
    d_unwind->add(node, d_result);
}

void CodeGenVisitor::visit(AstIf& node) {
    llvm::BasicBlock* trueBranch = createBlock("if_true");
    llvm::BasicBlock* falseBranch = createBlock("if_false");
    llvm::BasicBlock* cntBranch = createBlock("if_cnt");
    // Generate condition.
    llvm::Value* cond = visitExpression(*node.d_args->d_args[0]);
    d_builder->CreateCondBr(cond, trueBranch, falseBranch);
    // Generate true branch.
    d_builder->SetInsertPoint(trueBranch);
    llvm::Value* trueVal = visitExpression(*node.d_args->d_args[1]);
    d_builder->CreateBr(cntBranch);
    // Generate false branch.
    d_builder->SetInsertPoint(falseBranch);
    llvm::Value* falseVal = visitExpression(*node.d_args->d_args[2]);
    d_builder->CreateBr(cntBranch);
    // Generate merged branch (continue).
    d_builder->SetInsertPoint(cntBranch);
    assert(trueVal->getType() == falseVal->getType());
    llvm::PHINode* phiRes = d_builder->CreatePHI(trueVal->getType(), 2, "if_res");
    phiRes->addIncoming(trueVal, trueBranch);
    phiRes->addIncoming(falseVal, falseBranch);
    d_result = phiRes;
    d_unwind->add(node, d_result);
}

} // namespace jex
