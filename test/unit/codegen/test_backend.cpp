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
        "expr a : Integer = 123;\n"
        "expr b : Float = 123.456;\n");
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
        "expr a : Integer = 123 + 5 * (2 + 1);");
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
        "expr a : Integer = max3(1+1, 4+3, 2*2);");
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
        "expr a : Integer = if(1 < 2, 12+3, 0);", OptLevel::O0);
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
        "expr a : Integer = if(2 < 2, 12+3, 20-21);", OptLevel::O0);
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
        "expr a : String = \"Hello World!\";", OptLevel::O0);
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
        "expr a : String = substr(\"Hello World!\", 6, 5);", OptLevel::O0);
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
        R"(expr a : String = if(1 < 2, substr(substr("Hello World!", 6, 5), 0, 1), "Another string large enough to create an allocation");)", OptLevel::O0);
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
        R"(expr a : String = if(1 > 2, substr(substr("Hello World!", 6, 5), 0, 1), "Another string large enough to create an allocation");)", OptLevel::O0);
    std::unique_ptr<ExecutionContext> ctx = ExecutionContext::create(compiled);
    // Evaluate a.
    const uintptr_t fctAddr = compiled.getFctPtr("a");
    ASSERT_NE(0, fctAddr);
    auto fctA = reinterpret_cast<std::string* (*)(char*)>(fctAddr);
    ASSERT_EQ("Another string large enough to create an allocation", *fctA(ctx->getDataPtr()));
}

TEST(Backend, varExpr) {
    Environment env;
    env.addModule(BuiltInsModule());
    CompileResult compiled = compile(env,
        "expr a : Integer = 6; expr b : Integer = a + 1; expr c : Integer = a * b;", OptLevel::O0, true, false);
    std::unique_ptr<ExecutionContext> ctx = ExecutionContext::create(compiled);
    // Evaluate a.
    auto fctA = reinterpret_cast<int64_t* (*)(char*)>(compiled.getFctPtr("a"));
    auto fctB = reinterpret_cast<int64_t* (*)(char*)>(compiled.getFctPtr("b"));
    auto fctC = reinterpret_cast<int64_t* (*)(char*)>(compiled.getFctPtr("c"));
    ASSERT_TRUE(fctA != nullptr);
    ASSERT_TRUE(fctB != nullptr);
    ASSERT_TRUE(fctC != nullptr);
    ASSERT_EQ(6, *fctA(ctx->getDataPtr()));
    ASSERT_EQ(7, *fctB(ctx->getDataPtr()));
    ASSERT_EQ(42, *fctC(ctx->getDataPtr()));
}

TEST(Backend, varDef) {
    Environment env;
    env.addModule(BuiltInsModule());
    CompileResult compiled = compile(env,
        "var a : Integer; expr b : Integer = a + a; expr c : Integer = a * b;", OptLevel::O0, true, false);
    std::unique_ptr<ExecutionContext> ctx = ExecutionContext::create(compiled);
    // Store a.
    auto storeA = reinterpret_cast<void(*)(char*, int64_t*)>(compiled.getFctPtr("a"));
    int64_t value = 3;
    storeA(ctx->getDataPtr(), &value);
    // Evaluate b.
    auto fctB = reinterpret_cast<int64_t* (*)(char*)>(compiled.getFctPtr("b"));
    ASSERT_TRUE(fctB != nullptr);
    ASSERT_EQ(6, *fctB(ctx->getDataPtr()));
    // Evaluate c.
    auto fctC = reinterpret_cast<int64_t* (*)(char*)>(compiled.getFctPtr("c"));
    ASSERT_TRUE(fctC != nullptr);
    ASSERT_EQ(18, *fctC(ctx->getDataPtr()));
}

TEST(Backend, varDefComplexType) {
    Environment env;
    env.addModule(BuiltInsModule());
    CompileResult compiled = compile(env,
        "var a : String; expr b : String = substr(a, 6, 5);", OptLevel::O0, true, false);
    std::unique_ptr<ExecutionContext> ctx = ExecutionContext::create(compiled);
    // Store a.
    auto storeA = reinterpret_cast<void(*)(char*, std::string*)>(compiled.getFctPtr("a"));
    std::string str("Hello World");
    storeA(ctx->getDataPtr(), &str);
    // Evaluate b.
    auto fctB = reinterpret_cast<std::string* (*)(char*)>(compiled.getFctPtr("b"));
    ASSERT_TRUE(fctB != nullptr);
    ASSERT_EQ("World", *fctB(ctx->getDataPtr()));
    // Repeat.
    str = "1234567890";
    storeA(ctx->getDataPtr(), &str);
    ASSERT_EQ("7890", *fctB(ctx->getDataPtr()));
}

} // namespace jex
