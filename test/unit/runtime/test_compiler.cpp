#include <jex_backend.hpp>
#include <jex_builtins.hpp>
#include <jex_compiler.hpp>

#include <gtest/gtest.h>

#include <sstream>

namespace jex {

TEST(Compiler, emptyProgram) {
    Environment env;
    CompileResult res = Compiler::compile(env, "", OptLevel::O0);
    ASSERT_TRUE(res);
}

TEST(Compiler, compileError) {
    Environment env;
    CompileResult res = Compiler::compile(env, "1", OptLevel::O0);
    ASSERT_FALSE(res);
    std::stringstream errMsg;
    errMsg << res;
    ASSERT_EQ("1.1-1.1: Error: Unexpected integer literal '1', expecting 'var' or end of file", errMsg.str());
}

} // namespace jex
