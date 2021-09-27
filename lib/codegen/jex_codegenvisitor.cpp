#include <jex_codegenvisitor.hpp>

#include <jex_ast.hpp>
#include <jex_codemodule.hpp>
#include <jex_compileenv.hpp>
#include <jex_errorhandling.hpp>
#include <jex_intrinsicgen.hpp>
#include <jex_fctinfo.hpp>
#include <jex_symboltable.hpp>

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

void CodeGenVisitor::createIR() {
    d_offsets.clear();
    d_module = std::make_unique<CodeModule>(d_env);
    d_builder = std::make_unique<llvm::IRBuilder<>>(d_module->llvmContext());
    // Order by size descending, then by symbol name ascending.
    auto cmp = [](const Symbol* a, const Symbol* b) {
        return a->type->size() > b->type->size() ? true : a->name < b->name;
    };
    std::set<Symbol*, decltype(cmp)> vars(cmp);
    for (AstVariableDef* varDef: d_env.getRoot()->d_varDefs) {
        vars.insert(varDef->d_name->d_symbol);
    }
    size_t offset = 0;
    for (Symbol* sym : vars) {
        d_offsets.emplace(sym, offset);
        offset += sym->type->size();
    }
    d_env.setContextSize(offset);
    d_rctxType = llvm::StructType::create(d_module->llvmContext(), "Rctx");
    d_env.getRoot()->accept(*this);
}

llvm::Type* CodeGenVisitor::getType(TypeInfoId type) {
    auto[iter, inserted] = d_types.emplace(type, nullptr);
    if (inserted) {
        const TypeInfo::CreateTypeFct& fct = type->createTypeFct();
        if (!fct) {
            throw InternalError("Missing llvm::Type creator for type '" + type->name() + "'");
        }
        iter->second = fct(d_module->llvmContext());
    }
    return iter->second;
}

llvm::Value* CodeGenVisitor::visitExpression(IAstExpression& node) {
    assert(d_result == nullptr);
    node.accept(*this);
    assert(d_result != nullptr);
    return std::exchange(d_result, nullptr);
}

llvm::Value* CodeGenVisitor::getVarPtr(Symbol* varSym) {
    // Get rctx as i8*.
    llvm::Value* rctx = d_currFct->getArg(0);
    llvm::Type* bytePtrTy = llvm::IntegerType::get(d_module->llvmContext(), 8)->getPointerTo();
    llvm::Value* rctxAsI8Ptr = d_builder->CreateBitCast(rctx, bytePtrTy, "rctxAsBytePtr");
    // Apply offset to rctx pointer.
    llvm::Value* offset = llvm::ConstantInt::get(d_module->llvmContext(), llvm::APInt(64, d_offsets[varSym]));
    llvm::Value* varPtr = d_builder->CreateGEP(bytePtrTy->getPointerElementType(), rctxAsI8Ptr, offset, "varPtr");
    // Reinterpret cast to target type.
    return d_builder->CreateBitCast(varPtr, getType(varSym->type)->getPointerTo(), "varPtrTyped");
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
    // Current implementation will not work properly if type's underlying value is already a pointer type.
    if (result->getType()->isPointerTy()) {
        result = d_builder->CreateLoad(result, "resultLoaded");
    }
    llvm::Value* varPtr = getVarPtr(node.d_name->d_symbol);
    // TODO: Figure out copy operation: For large values, copy can't be done by a simple storeinst.
    d_builder->CreateStore(result, varPtr);
    d_builder->CreateRet(varPtr);
    // Link allocas block to begin block.
    d_builder->SetInsertPoint(allocaBlock);
    d_builder->CreateBr(blockBegin);
    d_currFct = nullptr;
}

void CodeGenVisitor::visit(AstLiteralExpr& node) {
    switch (node.d_resultType->kind()) {
        case TypeId::Integer:
            d_result = llvm::ConstantInt::get(d_module->llvmContext(), llvm::APInt(64, node.d_value.d_int));
            return;
        case TypeId::Float:
            d_result = llvm::ConstantFP::get(d_module->llvmContext(), llvm::APFloat(node.d_value.d_float));
            return;
        case TypeId::Bool:
            d_result = llvm::ConstantInt::get(d_module->llvmContext(), llvm::APInt(1, node.d_value.d_int));
            return;
        default:
            d_env.throwError(node.d_loc, "Literal not supported by code generation");
    }
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
