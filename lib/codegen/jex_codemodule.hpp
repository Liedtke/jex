#pragma once

#include <jex_compileenv.hpp>

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

namespace jex {

class CodeModule {
public:
    llvm::LLVMContext d_llvmContext;
    llvm::Module d_llvmModule;

    CodeModule(const CompileEnv& env)
    : d_llvmContext()
    , d_llvmModule(env.fileName(), d_llvmContext) {
    }
};

} // namespace jex
