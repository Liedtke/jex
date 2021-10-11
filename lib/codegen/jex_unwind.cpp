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
, d_unwindBegin(llvm::BasicBlock::Create(d_module.llvmContext(), "unwind", d_fct))
, d_unwindEnd(d_unwindBegin)
, d_newestBlock(d_unwindBegin) {
    d_builder->SetInsertPoint(d_unwindBegin);
}

void Unwind::add(IAstExpression& node, llvm::Value* value) {
    if (!requiresUnwind(node)) {
        return;
    }
    d_hasAnyUnwind = true;
    if (d_branches.empty()) {
        d_builder->SetInsertPoint(d_unwindBegin, d_unwindBegin->getFirstInsertionPt());
    } else {
        CondBranch& branch = d_branches.top();
        llvm::BasicBlock** block = branch.isA ? &branch.unwindBlockA : &branch.unwindBlockB;
        if (*block == nullptr) {
            *block = createBasicBlock("unwind");
        }
        d_builder->SetInsertPoint(*block, (*block)->getFirstInsertionPt());
    }
    TypeInfoId type = node.d_resultType;
    const FctInfo& dtor = d_env.fctLibrary().getFct("_dtor_" + type->name(), {});
    assert(dtor.d_retType == type && "destructor has invalid return type");
    llvm::FunctionCallee dtorCallee = d_utils.getOrCreateFct(&dtor);
    d_builder->CreateCall(dtorCallee, {value});
}

void Unwind::finalize(llvm::BasicBlock* insertPoint, llvm::Value* retVal) {
    assert(d_branches.empty() && "Unterminated branches in Unwinding");
    d_builder->SetInsertPoint(insertPoint);
    // If there isn't any unwinding, remove the unwinding completely for better code readability.
    if (!d_hasAnyUnwind) {
        assert(d_unwindBegin == d_unwindEnd);
        assert(d_unwindBegin->empty());
        d_builder->CreateRet(retVal);
        d_unwindEnd->eraseFromParent();
        d_unwindEnd = nullptr;
        return;
    }
    d_builder->CreateBr(d_unwindBegin);
    d_builder->SetInsertPoint(d_unwindEnd);
    d_builder->CreateRet(retVal);
}

void Unwind::initCondBranch(llvm::BranchInst* branchInst) {
    assert(branchInst->isConditional());
    d_branches.emplace(branchInst);
}

void Unwind::switchCondBranch(llvm::BranchInst* branchInst) {
    assert(!d_branches.empty() && "Missing branch");
    assert(d_branches.top().branchInst == branchInst && "Provided branch is not top of unwind branching");
    assert(d_branches.top().isA && "Duplicate switchCondBranch call");
    d_branches.top().isA = false;
}

void Unwind::leaveCondBranch(llvm::BranchInst* branchInst) {
    assert(!d_branches.empty() && "Missing branch");
    assert(d_branches.top().branchInst == branchInst && "Provided branch is not top of unwind branching");
    assert(!d_branches.top().isA && "Missing switchCondBranch call prior to leaveCondBranch");
    // Remove branch.
    CondBranch& branch = d_branches.top();
    d_branches.pop();
    // Generate unwind coding if required.
    if (branch.unwindBlockA || branch.unwindBlockB) {
        // Boolean flag whether block A was hit.
        llvm::Type* boolTy = llvm::Type::getInt1Ty(d_module.llvmContext());
        llvm::Value* flag = new llvm::AllocaInst(boolTy, 0, "unw_flag", &d_fct->getEntryBlock());
        // Set flag in original branches.
        llvm::Value* trueVal = llvm::ConstantInt::get(d_module.llvmContext(), llvm::APInt(1, 1));
        assert(!branch.branchInst->getSuccessor(0)->empty());
        assert(!branch.branchInst->getSuccessor(1)->empty());
        new llvm::StoreInst(trueVal, flag, &branch.branchInst->getSuccessor(0)->front());
        llvm::Value* falseVal = llvm::ConstantInt::get(d_module.llvmContext(), llvm::APInt(1, 0));
        new llvm::StoreInst(falseVal, flag, &branch.branchInst->getSuccessor(1)->front());
        llvm::BasicBlock** previousUnwind = &d_unwindBegin;
        if (!d_branches.empty()) {
            CondBranch& outerBranch = d_branches.top();
            previousUnwind = outerBranch.isA ? &outerBranch.unwindBlockA : &outerBranch.unwindBlockB;
            if (*previousUnwind == nullptr) {
                *previousUnwind = createBasicBlock("unwind");
            }
        }
        llvm::BasicBlock* unwindA = *previousUnwind;
        llvm::BasicBlock* unwindB = *previousUnwind;
        if (branch.unwindBlockA) {
            unwindA = branch.unwindBlockA;
            llvm::BranchInst::Create(d_unwindBegin, branch.unwindBlockA);
        }
        if (branch.unwindBlockB) {
            unwindB = branch.unwindBlockB;
            llvm::BranchInst::Create(d_unwindBegin, branch.unwindBlockB);
        }
        llvm::BasicBlock* newBlock = createBasicBlock("unwind");
        llvm::Value* flagLoaded = new llvm::LoadInst(flag->getType()->getPointerElementType(), flag, "flag_loaded", newBlock);
        llvm::BranchInst::Create(unwindA, unwindB, flagLoaded, newBlock);
        *previousUnwind = newBlock;
    }
}

llvm::BasicBlock* Unwind::createBasicBlock(const char* name) {
    return d_newestBlock = llvm::BasicBlock::Create(d_module.llvmContext(), name, d_fct, d_newestBlock);
}

} // namespace jex
