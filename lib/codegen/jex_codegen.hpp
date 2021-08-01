#pragma once

#include <memory>

namespace llvm {
    class Module;
}

namespace jex {

class CodeModule;
class CompileEnv;

class CodeGen {
    CompileEnv& d_env;
    std::unique_ptr<CodeModule> d_module;
public:
    CodeGen(CompileEnv& env);
    ~CodeGen();

    void createIR();

    const llvm::Module* getLlvmModule() const;
};

} // namespace jex
