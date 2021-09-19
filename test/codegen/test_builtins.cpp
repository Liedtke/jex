#include <jex_builtins.hpp>

#include <test_base.hpp>

#include <gtest/gtest.h>

namespace jex {

TEST(BuiltIns, Arithmetic) {
    Environment env;
    env.addModule(BuiltInsModule());
    CompileResult compiled = compile(env,
        "var a : Integer = 1 + 2 + 3 * 5 + 6 / 3 - 4 % 3;", OptLevel::O0);
    ASSERT_LE(8, compiled.getContextSize());
    auto ctx = std::make_unique<char[]>(compiled.getContextSize());
    // Evaluate a.
    const uintptr_t fctAddr = compiled.getFctPtr("a");
    ASSERT_NE(0, fctAddr);
    auto fctA = reinterpret_cast<int64_t* (*)(char*)>(fctAddr);
    ASSERT_EQ(19, *fctA(ctx.get()));
}

} // namespace jex
