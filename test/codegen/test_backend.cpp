#include <test_base.hpp>

#include <jex_builtins.hpp>
#include <jex_errorhandling.hpp>
#include <jex_executioncontext.hpp>

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
    std::unique_ptr<ExecutionContext> ctx = ExecutionContext::create(compiled);
    { // Evaluate a.
        const uintptr_t fctAddr = compiled.getFctPtr("a");
        ASSERT_NE(0, fctAddr);
        auto fctA = reinterpret_cast<int64_t* (*)(char*)>(fctAddr);
        ASSERT_EQ(123, *fctA(ctx->getDataPtr()));
    }
    { // Evaluate b.
        const uintptr_t fctAddr = compiled.getFctPtr("b");
        ASSERT_NE(0, fctAddr);
        auto fctA = reinterpret_cast<double* (*)(char*)>(fctAddr);
        ASSERT_DOUBLE_EQ(123.456, *fctA(ctx->getDataPtr()));
    }
    // c does not exist
    ASSERT_THROW(compiled.getFctPtr("c"), InternalError);
}

TEST(Backend, simpleCall) {
    Environment env;
    env.addModule(BuiltInsModule());
    CompileResult compiled = compile(env,
        "var a : Integer = 123 + 5 * (2 + 1);");
    std::unique_ptr<ExecutionContext> ctx = ExecutionContext::create(compiled);
    // Evaluate a.
    const uintptr_t fctAddr = compiled.getFctPtr("a");
    ASSERT_NE(0, fctAddr);
    auto fctA = reinterpret_cast<int64_t* (*)(char*)>(fctAddr);
    ASSERT_EQ(138, *fctA(ctx->getDataPtr()));
}

TEST(Backend, simpleFctCall) {
    Environment env;
    env.addModule(BuiltInsModule());
    env.addModule(TestModule());
    CompileResult compiled = compile(env,
        "var a : Integer = max3(1+1, 4+3, 2*2);");
    std::unique_ptr<ExecutionContext> ctx = ExecutionContext::create(compiled);
    // Evaluate a.
    const uintptr_t fctAddr = compiled.getFctPtr("a");
    ASSERT_NE(0, fctAddr);
    auto fctA = reinterpret_cast<int64_t* (*)(char*)>(fctAddr);
    ASSERT_EQ(7, *fctA(ctx->getDataPtr()));
}

TEST(Backend, ifExpressionTrue) {
    Environment env;
    env.addModule(BuiltInsModule());
    CompileResult compiled = compile(env,
        "var a : Integer = if(1 < 2, 12+3, 0);", OptLevel::O0);
    std::unique_ptr<ExecutionContext> ctx = ExecutionContext::create(compiled);
    // Evaluate a.
    const uintptr_t fctAddr = compiled.getFctPtr("a");
    ASSERT_NE(0, fctAddr);
    auto fctA = reinterpret_cast<int64_t* (*)(char*)>(fctAddr);
    ASSERT_EQ(15, *fctA(ctx->getDataPtr()));
}

TEST(Backend, ifExpressionFalse) {
    Environment env;
    env.addModule(BuiltInsModule());
    CompileResult compiled = compile(env,
        "var a : Integer = if(2 < 2, 12+3, 20-21);", OptLevel::O0);
    std::unique_ptr<ExecutionContext> ctx = ExecutionContext::create(compiled);
    // Evaluate a.
    const uintptr_t fctAddr = compiled.getFctPtr("a");
    ASSERT_NE(0, fctAddr);
    auto fctA = reinterpret_cast<int64_t* (*)(char*)>(fctAddr);
    ASSERT_EQ(-1, *fctA(ctx->getDataPtr()));
}

TEST(Backend, stringExpression) {
    Environment env;
    env.addModule(BuiltInsModule());
    CompileResult compiled = compile(env,
        "var a : String = \"Hello World!\";", OptLevel::O0);
    std::unique_ptr<ExecutionContext> ctx = ExecutionContext::create(compiled);
    // Evaluate a.
    const uintptr_t fctAddr = compiled.getFctPtr("a");
    ASSERT_NE(0, fctAddr);
    auto fctA = reinterpret_cast<std::string* (*)(char*)>(fctAddr);
    ASSERT_EQ("Hello World!", *fctA(ctx->getDataPtr()));
}

TEST(Backend, stringExpressionWithTemporary) {
    Environment env;
    env.addModule(BuiltInsModule());
    CompileResult compiled = compile(env,
        "var a : String = substr(\"Hello World!\", 6, 5);", OptLevel::O0);
    std::unique_ptr<ExecutionContext> ctx = ExecutionContext::create(compiled);
    // Evaluate a.
    const uintptr_t fctAddr = compiled.getFctPtr("a");
    ASSERT_NE(0, fctAddr);
    auto fctA = reinterpret_cast<std::string* (*)(char*)>(fctAddr);
    ASSERT_EQ("World", *fctA(ctx->getDataPtr()));
}

TEST(Backend, stringExpressionWithConditionalTemporary) {
    Environment env;
    env.addModule(BuiltInsModule());
    CompileResult compiled = compile(env,
        "var a : String = if(1 < 2, substr(substr(\"Hello World!\", 6, 5), 0, 1), \"Another string large enough to create an allocation\");", OptLevel::O0);
    std::unique_ptr<ExecutionContext> ctx = ExecutionContext::create(compiled);
    // Evaluate a.
    const uintptr_t fctAddr = compiled.getFctPtr("a");
    ASSERT_NE(0, fctAddr);
    auto fctA = reinterpret_cast<std::string* (*)(char*)>(fctAddr);
    ASSERT_EQ("W", *fctA(ctx->getDataPtr()));
}

TEST(Backend, stringExpressionWithConditionalTemporaryNotCreated) {
    Environment env;
    env.addModule(BuiltInsModule());
    CompileResult compiled = compile(env,
        "var a : String = if(1 > 2, substr(substr(\"Hello World!\", 6, 5), 0, 1), \"Another string large enough to create an allocation\");", OptLevel::O0);
    std::unique_ptr<ExecutionContext> ctx = ExecutionContext::create(compiled);
    // Evaluate a.
    const uintptr_t fctAddr = compiled.getFctPtr("a");
    ASSERT_NE(0, fctAddr);
    auto fctA = reinterpret_cast<std::string* (*)(char*)>(fctAddr);
    ASSERT_EQ("Another string large enough to create an allocation", *fctA(ctx->getDataPtr()));
}

} // namespace jex
