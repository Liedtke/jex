#include <test_base.hpp>

#include <jex_builtins.hpp>
#include <jex_errorhandling.hpp>

#include <gtest/gtest.h>

namespace jex {

static void max3(int64_t* res, int64_t a, int64_t b, int64_t c) {
    *res = std::max({a, b, c});
}

namespace {
class TestModule : public Module {
    void registerTypes(Registry& registry) const override {}
    void registerFcts(Registry& registry) const override {
        registry.registerFct(FctDesc<ArgInteger, ArgInteger, ArgInteger, ArgInteger>("max3", max3));
    }
};
}

TEST(Backend, simpleVarDef) {
    Environment env;
    env.addModule(BuiltInsModule());
    CompileResult compiled = compile(env,
        "var a : Integer = 123;\n"
        "var b : Float = 123.456;\n");
    ASSERT_LE(16, compiled.getContextSize());
    auto ctx = std::make_unique<char[]>(compiled.getContextSize());
    { // Evaluate a.
        const uintptr_t fctAddr = compiled.getFctPtr("a");
        ASSERT_NE(0, fctAddr);
        auto fctA = reinterpret_cast<int64_t* (*)(char*)>(fctAddr);
        ASSERT_EQ(123, *fctA(ctx.get()));
    }
    { // Evaluate b.
        const uintptr_t fctAddr = compiled.getFctPtr("b");
        ASSERT_NE(0, fctAddr);
        auto fctA = reinterpret_cast<double* (*)(char*)>(fctAddr);
        ASSERT_DOUBLE_EQ(123.456, *fctA(ctx.get()));
    }
    // c does not exist
    ASSERT_THROW(compiled.getFctPtr("c"), InternalError);
}

TEST(Backend, simpleCall) {
    Environment env;
    env.addModule(BuiltInsModule());
    CompileResult compiled = compile(env,
        "var a : Integer = 123 + 5 * (2 + 1);");
    ASSERT_LE(8, compiled.getContextSize());
    auto ctx = std::make_unique<char[]>(compiled.getContextSize());
    // Evaluate a.
    const uintptr_t fctAddr = compiled.getFctPtr("a");
    ASSERT_NE(0, fctAddr);
    auto fctA = reinterpret_cast<int64_t* (*)(char*)>(fctAddr);
    ASSERT_EQ(138, *fctA(ctx.get()));
}

TEST(Backend, simpleFctCall) {
    Environment env;
    env.addModule(BuiltInsModule());
    env.addModule(TestModule());
    CompileResult compiled = compile(env,
        "var a : Integer = max3(1+1, 4+3, 2*2);");
    ASSERT_LE(8, compiled.getContextSize());
    auto ctx = std::make_unique<char[]>(compiled.getContextSize());
    // Evaluate a.
    const uintptr_t fctAddr = compiled.getFctPtr("a");
    ASSERT_NE(0, fctAddr);
    auto fctA = reinterpret_cast<int64_t* (*)(char*)>(fctAddr);
    ASSERT_EQ(7, *fctA(ctx.get()));
}

TEST(Backend, ifExpressionTrue) {
    Environment env;
    env.addModule(BuiltInsModule());
    CompileResult compiled = compile(env,
        "var a : Integer = if(1 < 2, 12+3, 0);", OptLevel::O0);
    ASSERT_LE(8, compiled.getContextSize());
    auto ctx = std::make_unique<char[]>(compiled.getContextSize());
    // Evaluate a.
    const uintptr_t fctAddr = compiled.getFctPtr("a");
    ASSERT_NE(0, fctAddr);
    auto fctA = reinterpret_cast<int64_t* (*)(char*)>(fctAddr);
    ASSERT_EQ(15, *fctA(ctx.get()));
}

TEST(Backend, ifExpressionFalse) {
    Environment env;
    env.addModule(BuiltInsModule());
    CompileResult compiled = compile(env,
        "var a : Integer = if(2 < 2, 12+3, 20-21);", OptLevel::O0);
    ASSERT_LE(8, compiled.getContextSize());
    auto ctx = std::make_unique<char[]>(compiled.getContextSize());
    // Evaluate a.
    const uintptr_t fctAddr = compiled.getFctPtr("a");
    ASSERT_NE(0, fctAddr);
    auto fctA = reinterpret_cast<int64_t* (*)(char*)>(fctAddr);
    ASSERT_EQ(-1, *fctA(ctx.get()));
}

} // namespace jex
