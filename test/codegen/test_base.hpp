#pragma once

#include <jex_backend.hpp>
#include <jex_codegen.hpp>
#include <jex_codemodule.hpp>
#include <jex_compileenv.hpp>
#include <jex_environment.hpp>
#include <jex_parser.hpp>
#include <jex_typeinference.hpp>

// Ensure backwards compatibility for some older gtest versions.
#ifndef INSTANTIATE_TEST_SUITE_P
#define INSTANTIATE_TEST_SUITE_P(...) INSTANTIATE_TEST_CASE_P(__VA_ARGS__)
#endif

namespace jex {

static inline CompileResult compile(const Environment& env,
                                    const char* sourceCode,
                                    OptLevel op = OptLevel::O2,
                                    bool useIntrinsics = true) {
    CompileEnv compileEnv(env, useIntrinsics);
    Parser parser(compileEnv, sourceCode);
    parser.parse();
    TypeInference typeInference(compileEnv);
    typeInference.run();
    CodeGen codeGen(compileEnv, op);
    codeGen.createIR();
    Backend backend(compileEnv);
    return backend.jit(codeGen.releaseModule());
}

} // namespace jex
