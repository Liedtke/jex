#pragma once

#include <jex_base.hpp>

#include <memory>

namespace llvm {
    class Module;
}

namespace jex {

class CodeModule;
class CompileEnv;

enum class OptLevel {
    O0 = 0, O1, O2, O3
};

class CodeGen : NoCopy {
    CompileEnv& d_env;
    std::unique_ptr<CodeModule> d_module;
    OptLevel d_optLevel;

public:
    CodeGen(CompileEnv& env, OptLevel optLevel);
    ~CodeGen();

    void createIR();
    void printIR(std::ostream& out);
    const llvm::Module& getLlvmModule() const;
    std::unique_ptr<CodeModule> releaseModule();

private:
    void optimize();
};

} // namespace jex
