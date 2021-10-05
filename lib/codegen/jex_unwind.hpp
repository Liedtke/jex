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
    CompileEnv& d_env;
    CodeModule& d_module;
    CodeGenUtils& d_utils;
    llvm::Function* d_fct;
    std::unique_ptr<llvm::IRBuilder<>> d_builder;
    llvm::BasicBlock* d_unwindEntry;

public:
    Unwind(CompileEnv& env, CodeModule& module, CodeGenUtils& utils, llvm::Function* fct);

    void add(IAstExpression& node, llvm::Value* value);
    void finalize(llvm::BasicBlock* insertPoint, llvm::Value* retVal);

    llvm::BasicBlock* getEntryBlock() const {
        return d_unwindEntry;
    }
};


} // namespace jex
