#include <jex_codemodule.hpp>

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

namespace jex {

CodeModule::CodeModule(const CompileEnv& env)
: d_llvmContext(std::make_unique<llvm::LLVMContext>())
, d_llvmModule(std::make_unique<llvm::Module>(env.fileName(), *d_llvmContext)) {
}

CodeModule::~CodeModule() {
}

std::unique_ptr<llvm::LLVMContext> CodeModule::releaseContext() {
    return std::move(d_llvmContext);
}

std::unique_ptr<llvm::Module> CodeModule::releaseModule() {
    return std::move(d_llvmModule);
}

} // namespace jex
