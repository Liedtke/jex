#pragma once

#include <jex_typeinfo.hpp>

#include "llvm/IR/IRBuilder.h"

#include <memory>
#include <stack>

namespace jex {

class CodeGenUtils;
class CodeModule;
class CompileEnv;
class IAstExpression;

class Unwind : NoCopy {
    struct BlockSeq {
        llvm::BasicBlock* begin = nullptr;
        llvm::BasicBlock* end = nullptr;

        BlockSeq() = default;
        explicit BlockSeq(llvm::BasicBlock* block) : begin(block), end(block) {
        }

        operator bool() {
            return begin != nullptr;
        }
    };
    struct CondBranch {
        llvm::BranchInst* branchInst;
        BlockSeq unwindA;
        BlockSeq unwindB;
        bool isA = true;

        explicit CondBranch(llvm::BranchInst* branchInst) : branchInst(branchInst), unwindA(), unwindB() {
        }
    };

    CompileEnv& d_env;
    CodeModule& d_module;
    CodeGenUtils& d_utils;
    llvm::Function* d_fct;
    std::unique_ptr<llvm::IRBuilder<>> d_builder;
    BlockSeq d_unwind;
    llvm::BasicBlock* d_newestBlock; // Latest created block, only used for ordering.
    std::stack<CondBranch> d_branches;
    bool d_hasAnyUnwind = false;

public:
    Unwind(CompileEnv& env, CodeModule& module, CodeGenUtils& utils, llvm::Function* fct);

    void add(IAstExpression& node, llvm::Value* value);
    void finalize(llvm::BasicBlock* insertPoint, llvm::Value* retVal);

    void initCondBranch(llvm::BranchInst* branchInst);
    void switchCondBranch(llvm::BranchInst* branchInst);
    void leaveCondBranch(llvm::BranchInst* branchInst);

    llvm::BasicBlock* getNewestBlock() const {
        return d_newestBlock;
    }

private:
    llvm::BasicBlock* createBasicBlock(const char* name);
};


} // namespace jex
