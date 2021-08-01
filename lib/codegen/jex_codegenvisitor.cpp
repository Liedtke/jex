#include <jex_codegenvisitor.hpp>

#include <jex_ast.hpp>
#include <jex_codemodule.hpp>

namespace jex {

CodeGenVisitor::CodeGenVisitor(CompileEnv& env)
: d_env(env)
, d_module() {
}

CodeGenVisitor::~CodeGenVisitor() {
}

void CodeGenVisitor::createIR() {
    d_module = std::make_unique<CodeModule>(d_env);
    d_env.getRoot()->accept(*this);
}

} // namespace jex
