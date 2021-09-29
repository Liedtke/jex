#include <jex_codegenvisitor.hpp>

#include <jex_ast.hpp>
#include <jex_codemodule.hpp>
#include <jex_constantstore.hpp>
#include <jex_compileenv.hpp>
#include <jex_errorhandling.hpp>
#include <jex_intrinsicgen.hpp>
#include <jex_fctinfo.hpp>
#include <jex_symboltable.hpp>

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

void CodeGenVisitor::createInit(const Symbol* sym) {
    llvm::Value* var = getVarPtr(sym);
    TypeInfoId type = sym->type;
    if (type->isZeroInitialized()) {
        // Simplified initialization instead of function call.
        d_builder->CreateStore(llvm::Constant::getNullValue(var->getType()->getPointerElementType()), var);
        return;
    }
    // Create constructor if not yet created.
    assert(type->lifetimeFcts().defaultConstructor && "Default constructor required");
    std::string fctName = type->name() + "__ctor";
    llvm::Function* fct = d_module->llvmModule().getFunction(fctName);
    if (!fct) {
        llvm::Type* argType = getType(type)->getPointerTo(); // Calling convention expects pointer to object.
        llvm::FunctionType* fctType = llvm::FunctionType::get(llvm::Type::getVoidTy(d_module->llvmContext()), {argType}, false);
        auto linkage = llvm::GlobalValue::LinkageTypes::InternalLinkage;
        fct = llvm::Function::Create(fctType, linkage, fctName, d_module->llvmModule());
    }
    d_builder->CreateCall(fct->getFunctionType(), fct, {var});
}

void CodeGenVisitor::createDestruct(const Symbol* sym) {
    TypeInfoId type = sym->type;
    if (type->kind() == TypeKind::Value) {
        return; // Nothing to do for value types.
    }
    assert(type->kind() == TypeKind::Complex &&
           "The context may only contain value and complex types");
    // Create destructor if not yet created.
    assert(type->lifetimeFcts().destructor && "Destructor required");
    std::string fctName = type->name() + "__dtor";
    llvm::Function* fct = d_module->llvmModule().getFunction(fctName);
    if (!fct) {
        llvm::Type* argType = getType(type)->getPointerTo(); // Calling convention expects pointer to object.
        llvm::FunctionType* fctType = llvm::FunctionType::get(llvm::Type::getVoidTy(d_module->llvmContext()), {argType}, false);
        auto linkage = llvm::GlobalValue::LinkageTypes::InternalLinkage;
        fct = llvm::Function::Create(fctType, linkage, fctName, d_module->llvmModule());
    }
    d_builder->CreateCall(fct->getFunctionType(), fct, {getVarPtr(sym)});

}

template<typename Iter>
void CodeGenVisitor::createInitDestructFct(Iter symBegin, Iter symEnd, const char* prefix,
                                           void(CodeGenVisitor::*createCall)(const Symbol*)) {
    // Create Function.
    llvm::Type* voidTy = llvm::Type::getVoidTy(d_module->llvmContext());
    llvm::Type* rctxPtrTy = d_module->llvmModule().getTypeByName("Rctx")->getPointerTo();
    llvm::FunctionType* fctType = llvm::FunctionType::get(voidTy, {rctxPtrTy}, false);
    d_currFct = llvm::Function::Create(
        fctType, llvm::GlobalValue::LinkageTypes::ExternalLinkage, llvm::Twine(prefix) + "_rctx", d_module->llvmModule());
    d_currFct->getArg(0)->setName("rctx");
    d_builder->SetInsertPoint(llvm::BasicBlock::Create(d_module->llvmContext(), "entry", d_currFct));
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

llvm::StructType* CodeGenVisitor::createOpaqueStructType(TypeInfoId type) {
    assert(type->size() % type->alignment() == 0 && "Type size has to be a multiple of its alignment");
    size_t numElements = type->size() / type->alignment();
    llvm::Type* intTy = llvm::IntegerType::get(d_module->llvmContext(), type->alignment() * 8);
    return llvm::StructType::create(std::vector<llvm::Type*>(numElements, intTy), type->name());
}

llvm::Type* CodeGenVisitor::getType(TypeInfoId type) {
    auto[iter, inserted] = d_types.emplace(type, nullptr);
    if (inserted) {
        const TypeInfo::CreateTypeFct& fct = type->createTypeFct();
        if (fct) {
            iter->second = fct(d_module->llvmContext());
        } else {
            assert(!d_module->llvmModule().getTypeByName(type->name()));
            iter->second = createOpaqueStructType(type);
        }
    }
    return iter->second;
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
    return d_builder->CreatePointerCast(varPtr, getType(varSym->type)->getPointerTo(), "varPtrTyped");
}

void CodeGenVisitor::createAssign(llvm::Value* result, llvm::Value* source, TypeInfoId type) {
    assert(result->getType() == source->getType() && "Assign expects two pointers of the same type");
    assert(type->lifetimeFcts().assign && "Assignment required");
    // Create function if not yet created.
    std::string fctName = type->name() + "__assign";
    llvm::Function* fct = d_module->llvmModule().getFunction(fctName);
    if (!fct) {
        llvm::Type* argType = getType(type)->getPointerTo(); // Calling convention expects pointer to object.
        llvm::FunctionType* fctType = llvm::FunctionType::get(llvm::Type::getVoidTy(d_module->llvmContext()), {argType, argType}, false);
        auto extLinkage = llvm::GlobalValue::LinkageTypes::InternalLinkage;
        fct = llvm::Function::Create(fctType, extLinkage, fctName, d_module->llvmModule());
    }
    d_builder->CreateCall(fct->getFunctionType(), fct, {result, source});
}

void CodeGenVisitor::visit(AstVariableDef& node) {
    assert(d_currFct == nullptr);
    // Create function.
    llvm::Type* resultPtrType = getType(node.d_type->d_resultType)->getPointerTo();
    llvm::FunctionType* fctType = llvm::FunctionType::get(resultPtrType, {d_rctxType->getPointerTo()}, false);
    d_currFct = llvm::Function::Create(
        fctType, llvm::GlobalValue::LinkageTypes::ExternalLinkage, toLlvm(node.d_name->d_name), d_module->llvmModule());
    d_currFct->getArg(0)->setName("rctx");
    llvm::BasicBlock* allocaBlock = llvm::BasicBlock::Create(d_module->llvmContext(), "entry", d_currFct);
    llvm::BasicBlock* blockBegin = llvm::BasicBlock::Create(d_module->llvmContext(), "begin", d_currFct);
    d_builder->SetInsertPoint(blockBegin);
    // Evaluate expression and store result.
    llvm::Value* result = visitExpression(*node.d_expr);
    // FIXME: Figure out how to handle different calling conventions.
    llvm::Value* varPtr = getVarPtr(node.d_name->d_symbol);
    if (node.d_resultType->kind() == TypeKind::Complex) {
        // TODO: Figure out if varPtr is temporary and generate move assign instead.
        createAssign(result, varPtr, node.d_resultType);
    } else {
        // Perform a simple store.
        // TODO: This might not be the right thing to do for large values.
        d_builder->CreateStore(result, varPtr);
    }
    d_builder->CreateRet(varPtr);
    // Link allocas block to begin block.
    d_builder->SetInsertPoint(allocaBlock);
    d_builder->CreateBr(blockBegin);
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
            const std::string* constStr = d_env.constants().emplace<std::string>(val);
            llvm::Type* strType = getType(d_env.typeSystem().getType("String"));
            llvm::Value* var = new llvm::GlobalVariable(d_module->llvmModule(), strType, /*isConstant*/true,
                llvm::GlobalValue::LinkageTypes::ExternalLinkage, nullptr,
                llvm::formatv("strLit_l{0}_c{1}", node.d_loc.begin.line, node.d_loc.begin.col));
            (void)constStr; // FIXME: Store mapping from name to constant pointer somewhere to link later on.
            return var;
        }
    }, node.d_value);
}

llvm::FunctionCallee CodeGenVisitor::getOrCreateFct(const FctInfo* fctInfo) {
    // Declare the C function.
    llvm::Type* voidTy = llvm::Type::getVoidTy(d_module->llvmContext());
    std::vector<llvm::Type*> params;
    // TODO: Design proper calling conventions.
    params.push_back(getType(fctInfo->d_retType)->getPointerTo());
    for (TypeInfoId paramType : fctInfo->d_paramTypes) {
        params.push_back(getType(paramType));
    }
    llvm::FunctionType* fctType = llvm::FunctionType::get(voidTy, params, false);
    if (d_env.useIntrinsics() && fctInfo->d_intrinsicFct) {
        // Generate and insert intrinsic function.
        llvm::Function* fct = d_module->llvmModule().getFunction(fctInfo->d_intrinsicName);
        if (fct == nullptr) {
            // Generate intrinsic.
            fct = llvm::Function::Create(
        fctType, llvm::GlobalValue::LinkageTypes::InternalLinkage, fctInfo->d_intrinsicName, d_module->llvmModule());
            IntrinsicGen intrinsicGen(*d_module, *fct);
            fctInfo->d_intrinsicFct(intrinsicGen);
        }
        return fct;
    } else {
        // Generate C function call.
        d_env.addFctUsage(fctInfo);
        return d_module->llvmModule().getOrInsertFunction(fctInfo->d_mangledName, fctType);
    }
}

void CodeGenVisitor::visit(AstBinaryExpr& node) {
    // Generate argument evaluation.
    llvm::Value* lhs = visitExpression(*node.d_lhs);
    llvm::Value* rhs = visitExpression(*node.d_rhs);
    // Generate alloca to store the result.
    llvm::Type* resType = getType(node.d_resultType);
    llvm::Value* res = new llvm::AllocaInst(resType, 0, "res_" + node.d_fctInfo->d_name, &d_currFct->getEntryBlock());
    llvm::FunctionCallee fct = getOrCreateFct(node.d_fctInfo);
    // Call the function.
    d_builder->CreateCall(fct.getFunctionType(), fct.getCallee(), {res, lhs, rhs});
    d_result = d_builder->CreateLoad(res);
}

void CodeGenVisitor::visit(AstFctCall& node) {
    // Generate alloca to store the result.
    llvm::Type* resType = getType(node.d_resultType);
    llvm::Value* res = new llvm::AllocaInst(resType, 0, "res_" + node.d_fctInfo->d_name, &d_currFct->getEntryBlock());
    llvm::FunctionCallee fct = getOrCreateFct(node.d_fctInfo);
    // Visit arguments.
    std::vector<llvm::Value*> args({res});
    for (IAstExpression* expr : node.d_args->d_args) {
        args.push_back(visitExpression(*expr));
    }
    // Call the function.
    d_builder->CreateCall(fct.getFunctionType(), fct.getCallee(), args);
    d_result = d_builder->CreateLoad(res);
}

void CodeGenVisitor::visit(AstIf& node) {
    llvm::BasicBlock* trueBranch = llvm::BasicBlock::Create(d_module->llvmContext(), "if_true", d_currFct);
    llvm::BasicBlock* falseBranch = llvm::BasicBlock::Create(d_module->llvmContext(), "if_false", d_currFct);
    llvm::BasicBlock* cntBranch = llvm::BasicBlock::Create(d_module->llvmContext(), "if_cnt", d_currFct);
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
}

} // namespace jex
