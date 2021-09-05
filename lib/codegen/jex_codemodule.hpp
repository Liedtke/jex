#pragma once

#include <jex_base.hpp>

#include <memory>

namespace llvm {
    class LLVMContext;
    class Module;
}

namespace jex {

class CompileEnv;

class CodeModule : NoCopy {
    std::unique_ptr<llvm::LLVMContext> d_llvmContext;
    std::unique_ptr<llvm::Module> d_llvmModule;

public:
    CodeModule(const CompileEnv& env);
    ~CodeModule();

    llvm::LLVMContext& llvmContext() {
        return *d_llvmContext;
    }

    llvm::Module& llvmModule() {
        return *d_llvmModule;
    }

    std::unique_ptr<llvm::LLVMContext> releaseContext();
    std::unique_ptr<llvm::Module> releaseModule();
};

} // namespace jex
