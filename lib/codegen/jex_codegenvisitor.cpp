#include <jex_codegenvisitor.hpp>

#include <jex_ast.hpp>
#include <jex_codemodule.hpp>
#include <jex_errorhandling.hpp>
#include <jex_symboltable.hpp>

#include <set>

namespace jex {

llvm::StringRef toLlvm(std::string_view str) {
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
    d_builder = std::make_unique<llvm::IRBuilder<>>(d_module->d_llvmContext);
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
    d_rctxType = llvm::StructType::create(d_module->d_llvmContext, "Rctx");
    d_env.getRoot()->accept(*this);
}

llvm::Type* CodeGenVisitor::getType(TypeInfoId type) {
    auto[iter, inserted] = d_types.emplace(type, nullptr);
    if (inserted) {
        const TypeInfo::CreateTypeFct& fct = type->createTypeFct();
        if (!fct) {
            throw InternalError("Missing llvm::Type creator for type '" + type->name() + "'");
        }
        iter->second = fct(d_module->d_llvmContext);
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
    llvm::Value* rctx = d_currFct->getArg(0);
    llvm::Type* bytePtrTy = llvm::IntegerType::get(d_module->d_llvmContext, 8)->getPointerTo();
    llvm::Value* rctxAsI8Ptr = d_builder->CreateBitCast(rctx, bytePtrTy, "rctxAsBytePtr");
    llvm::Value* offset = llvm::ConstantInt::get(d_module->d_llvmContext, llvm::APInt(64, d_offsets[varSym]));
    llvm::Value* varPtr = d_builder->CreateGEP(bytePtrTy->getPointerElementType(), rctxAsI8Ptr, offset, "varPtr");
    return d_builder->CreateBitCast(varPtr, getType(varSym->type)->getPointerTo(), "varPtrTyped");
}

void CodeGenVisitor::visit(AstVariableDef& node) {
    assert(d_currFct == nullptr);
    // Create function.
    llvm::Type* resultPtrType = getType(node.d_type->d_resultType)->getPointerTo();
    llvm::FunctionType* fctType = llvm::FunctionType::get(resultPtrType, {d_rctxType->getPointerTo()}, false);
    d_currFct = llvm::Function::Create(
        fctType, llvm::GlobalValue::LinkageTypes::ExternalLinkage, toLlvm(node.d_name->d_name), d_module->d_llvmModule);
    d_currFct->getArg(0)->setName("rctx");
    llvm::BasicBlock* block = llvm::BasicBlock::Create(d_module->d_llvmContext, "entry", d_currFct);
    d_builder->SetInsertPoint(block);
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
    d_currFct = nullptr;
}

void CodeGenVisitor::visit(AstLiteralExpr& node) {
    switch (node.d_resultType->kind()) {
        case TypeId::Integer:
            d_result = llvm::ConstantInt::get(d_module->d_llvmContext, llvm::APInt(64, node.d_value.d_int));
            return;
        case TypeId::Float:
            d_result = llvm::ConstantFP::get(d_module->d_llvmContext, llvm::APFloat(node.d_value.d_float));
            return;
        case TypeId::Bool:
            d_result = llvm::ConstantInt::get(d_module->d_llvmContext, llvm::APInt(1, node.d_value.d_int));
            return;
        default:
            d_env.throwError(node.d_loc, "Literal not supported by code generation");
    }
}

} // namespace jex
