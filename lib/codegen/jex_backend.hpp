#pragma once

#include <jex_base.hpp>

#include <memory>

namespace llvm::orc {
    class LLJIT;
    class ThreadSafeModule;
}

namespace jex {

class CodeModule;
class CompileEnv;

class Backend :  NoCopy {
    CompileEnv& d_env;
    std::unique_ptr<llvm::orc::LLJIT> d_jit;
public:
    static void initialize();

    Backend(CompileEnv& env);
    ~Backend();

    void jit(std::unique_ptr<CodeModule> module);
    uintptr_t getFctPtr(std::string_view fctName);
};

} // namespace jex
