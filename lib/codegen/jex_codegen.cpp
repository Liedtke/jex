#include <jex_codegen.hpp>

#include <jex_codegenvisitor.hpp>
#include <jex_codemodule.hpp>

namespace jex {

CodeGen::CodeGen(CompileEnv& env)
: d_env(env)
, d_module() {
}

CodeGen::~CodeGen() {
}

const llvm::Module* CodeGen::getLlvmModule() const {
    return &d_module->d_llvmModule;
}

void CodeGen::createIR() {
    CodeGenVisitor codeGenVisitor(d_env);
    codeGenVisitor.createIR();
    d_module = codeGenVisitor.releaseModule();
}

} // namespace jex
