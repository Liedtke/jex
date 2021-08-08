#pragma once

#include <jex_base.hpp>
#include <jex_basicastvisitor.hpp>

#include <memory>

namespace jex {

class CodeModule;
class CompileEnv;

class CodeGenVisitor : private BasicAstVisitor, NoCopy {
    CompileEnv& d_env;
    std::unique_ptr<CodeModule> d_module;
public:
    CodeGenVisitor(CompileEnv& env);
    ~CodeGenVisitor();

    void createIR();
    std::unique_ptr<CodeModule> releaseModule() {
        return std::move(d_module);
    }
};

} // namespace jex
