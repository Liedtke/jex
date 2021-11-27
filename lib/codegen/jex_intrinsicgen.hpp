#pragma once

#include <jex_base.hpp>
#include <jex_codemodule.hpp>

#include "llvm/IR/IRBuilder.h"

namespace jex {

class CodeModule;

class IntrinsicGen : NoCopy {
    CodeModule&     d_codeModule;
    llvm::Function& d_fct;
    std::unique_ptr<llvm::IRBuilder<>> d_builder;
public:
    IntrinsicGen(CodeModule& codeModule, llvm::Function& fct);
    ~IntrinsicGen();

    llvm::Function& fct() {
        return d_fct;
    }

    llvm::LLVMContext& llvmContext() {
        return d_codeModule.llvmContext();
    }

    llvm::Module& llvmModule() {
        return d_codeModule.llvmModule();
    }

    llvm::IRBuilder<>& builder() {
        return *d_builder;
    }

    llvm::Value* getStructElemPtr(llvm::Value* structPtr, int index, const llvm::Twine& name = "");
};

} // namespace jex
