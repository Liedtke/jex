#include <jex_unwind.hpp>

#include <jex_ast.hpp>
#include <jex_codegenutils.hpp>
#include <jex_codemodule.hpp>
#include <jex_compileenv.hpp>
#include <jex_fctinfo.hpp>
#include <jex_fctlibrary.hpp>

namespace jex {

static bool requiresUnwind(const IAstExpression& expr) {
    return expr.isTemporary() && expr.d_resultType->kind() == TypeKind::Complex;
}

Unwind::Unwind(CompileEnv& env, CodeModule& module, CodeGenUtils& utils, llvm::Function* fct)
: d_env(env)
, d_module(module)
, d_utils(utils)
, d_fct(fct)
, d_builder(std::make_unique<llvm::IRBuilder<>>(module.llvmContext()))
, d_unwindEntry(llvm::BasicBlock::Create(d_module.llvmContext(), "unwind", d_fct)) {
    d_builder->SetInsertPoint(d_unwindEntry);
}

void Unwind::add(IAstExpression& node, llvm::Value* value) {
    if (!requiresUnwind(node)) {
        return;
    }
    TypeInfoId type = node.d_resultType;
    const FctInfo& dtor = d_env.fctLibrary().getFct("_dtor_" + type->name(), {});
    assert(dtor.d_retType == type && "destructor has invalid return type");
    llvm::FunctionCallee dtorCallee = d_utils.getOrCreateFct(&dtor);
    llvm::Instruction* inst = d_builder->CreateCall(dtorCallee, {value});
    d_builder->SetInsertPoint(inst); // Backwards insertion.
}

void Unwind::finalize(llvm::BasicBlock* insertPoint, llvm::Value* retVal) {
    d_builder->SetInsertPoint(insertPoint);
    // If there isn't any unwinding, remove the unwinding completely for better code readability.
    if (d_unwindEntry->empty()) {
        d_builder->CreateRet(retVal);
        d_unwindEntry->eraseFromParent();
        d_unwindEntry = nullptr;
        return;
    }
    d_builder->CreateBr(d_unwindEntry);
    // TODO: Handle different block due to required control flow for if expression.
    d_builder->SetInsertPoint(d_unwindEntry);
    d_builder->CreateRet(retVal);
}

} // namespace jex
