#pragma once

#include <jex_base.hpp>

#include <memory>

namespace llvm {
    class Module;
}

namespace jex {

class CodeModule;
class CompileEnv;

class CodeGen : NoCopy {
    CompileEnv& d_env;
    std::unique_ptr<CodeModule> d_module;
public:
    CodeGen(CompileEnv& env);
    ~CodeGen();

    void createIR();

    const llvm::Module& getLlvmModule() const;

    std::unique_ptr<CodeModule> releaseModule() {
        return std::move(d_module);
    }
};

} // namespace jex
