#include <jex_backend.hpp>
#include <jex_builtins.hpp>
#include <jex_codegen.hpp>
#include <jex_codemodule.hpp>
#include <jex_compileenv.hpp>
#include <jex_environment.hpp>
#include <jex_errorhandling.hpp>
#include <jex_parser.hpp>
#include <jex_typeinference.hpp>

#include <gtest/gtest.h>

namespace jex {

TEST(Backend, simpleVarDef) {
    Environment env;
    env.addModule(BuiltInsModule());
    CompileEnv compileEnv(env);
    Parser parser(compileEnv,
    "var a : Integer = 123;\n"
    "var b : Float = 123.456;\n");
    parser.parse();
    TypeInference typeInference(compileEnv);
    typeInference.run();
    CodeGen codeGen(compileEnv, OptLevel::O0);
    codeGen.createIR();
    Backend backend(compileEnv);
    ASSERT_LE(16, compileEnv.getContextSize());
    auto ctx = std::make_unique<char[]>(compileEnv.getContextSize());
    backend.jit(codeGen.releaseModule());
    { // Evaluate a.
        const uintptr_t fctAddr = backend.getFctPtr("a");
        ASSERT_NE(0, fctAddr);
        auto fctA = reinterpret_cast<int64_t* (*)(char*)>(fctAddr);
        ASSERT_EQ(123, *fctA(ctx.get()));
    }
    { // Evaluate b.
        const uintptr_t fctAddr = backend.getFctPtr("b");
        ASSERT_NE(0, fctAddr);
        auto fctA = reinterpret_cast<double* (*)(char*)>(fctAddr);
        ASSERT_DOUBLE_EQ(123.456, *fctA(ctx.get()));
    }
    // c does not exist
    ASSERT_THROW(backend.getFctPtr("c"), InternalError);
}

TEST(Backend, simpleCall) {
    Environment env;
    env.addModule(BuiltInsModule());
    CompileEnv compileEnv(env);
    Parser parser(compileEnv, "var a : Integer = 123 + 5 * (2 + 1);");
    parser.parse();
    TypeInference typeInference(compileEnv);
    typeInference.run();
    CodeGen codeGen(compileEnv, OptLevel::O0);
    codeGen.createIR();
    Backend backend(compileEnv);
    ASSERT_LE(8, compileEnv.getContextSize());
    auto ctx = std::make_unique<char[]>(compileEnv.getContextSize());
    backend.jit(codeGen.releaseModule());
    { // Evaluate a.
        const uintptr_t fctAddr = backend.getFctPtr("a");
        ASSERT_NE(0, fctAddr);
        auto fctA = reinterpret_cast<int64_t* (*)(char*)>(fctAddr);
        ASSERT_EQ(138, *fctA(ctx.get()));
    }
}

} // namespace jex
