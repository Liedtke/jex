#include <jex_compiler.hpp>

#include <jex_compileenv.hpp>
#include <jex_parser.hpp>
#include <jex_typeinference.hpp>
#include <jex_codegen.hpp>
#include <jex_codemodule.hpp>
#include <jex_backend.hpp>
#include <jex_errorhandling.hpp>

namespace jex {

CompileResult Compiler::compile(Environment& env, const std::string& source, OptLevel optLevel, bool useIntrinsics) {
    CompileEnv compileEnv(env, useIntrinsics);
    try {
        // 1) Parse.
        Parser parser(compileEnv, source.c_str());
        parser.parse();
        // 2) Type inference.
        TypeInference typeInference(compileEnv);
        typeInference.run();
        // 3) Code generation (LLVM IR).
        CodeGen codeGen(compileEnv, optLevel);
        codeGen.createIR();
        // 4) Create llvm JIT.
        Backend backend(compileEnv);
        return backend.jit(codeGen.releaseModule());
    } catch (const CompileError&) {
        assert(compileEnv.hasErrors());
        assert(!compileEnv.messages().empty());
        return CompileResult(compileEnv.releaseMessages());
    }
}

} // namespace jex
