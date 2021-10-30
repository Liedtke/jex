#include <jex_compiler.hpp>

#include <jex_compileenv.hpp>
#include <jex_parser.hpp>
#include <jex_typeinference.hpp>
#include <jex_codegen.hpp>
#include <jex_codemodule.hpp>
#include <jex_constantfolding.hpp>
#include <jex_backend.hpp>
#include <jex_builtins.hpp>
#include <jex_environment.hpp>
#include <jex_errorhandling.hpp>

namespace jex {

static void parseAndCheck(CompileEnv& compileEnv, const std::string& source, bool enableConstantFolding) {
    Parser parser(compileEnv, source.c_str());
    parser.parse();
    TypeInference typeInference(compileEnv);
    typeInference.run();
    if (enableConstantFolding) {
        ConstantFolding constFolding(compileEnv);
        constFolding.run();
    }
}

CompileResult Compiler::compile(const Environment& env, const std::string& source, OptLevel optLevel, bool useIntrinsics, bool enableConstantFolding) {
    CompileEnv compileEnv(env, useIntrinsics);
    try {
        parseAndCheck(compileEnv, source, enableConstantFolding);
        CodeGen codeGen(compileEnv, optLevel);
        codeGen.createIR();
        Backend backend(compileEnv);
        return backend.jit(codeGen.releaseModule());
    } catch (const CompileError&) {
        assert(compileEnv.hasErrors());
        assert(!compileEnv.messages().empty());
        return CompileResult(compileEnv.releaseMessages());
    }
}

void Compiler::printIR(std::ostream& out, const Environment& env, const std::string& source, OptLevel optLevel, bool useIntrinsics, bool enableConstantFolding) {
    CompileEnv compileEnv(env, useIntrinsics);
    parseAndCheck(compileEnv, source, enableConstantFolding);
    CodeGen codeGen(compileEnv, optLevel);
    codeGen.createIR();
    codeGen.printIR(out);
}

} // namespace jex
