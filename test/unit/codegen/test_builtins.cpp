#include <jex_builtins.hpp>
#include <jex_executioncontext.hpp>

#include <test_base.hpp>

#include <gtest/gtest.h>

#include <limits>
#include <variant>

namespace jex {

using EvalVariant = std::variant<int64_t, double, bool, std::string>;
using TestEvalT = std::pair<const char*, EvalVariant>;
class TestEval : public testing::TestWithParam<TestEvalT> {};

static void testEval(const char *code, const EvalVariant& exp, bool useIntrinsics, bool runConstFolding) {
    Environment env;
    env.addModule(BuiltInsModule());
    CompileResult compiled = compile(env, code, OptLevel::O1, useIntrinsics, runConstFolding);
    std::unique_ptr<ExecutionContext> ctx = ExecutionContext::create(compiled);
    // Evaluate a.
    const uintptr_t fctAddr = compiled.getFctPtr("a");
    ASSERT_NE(0, fctAddr);
    auto fctA = reinterpret_cast<void* (*)(char*)>(fctAddr);
    std::visit([&](auto exp) {
        auto act = *reinterpret_cast<decltype(exp)*>(fctA(ctx->getDataPtr()));
        if constexpr (std::is_same_v<decltype(exp), double>) {
            ASSERT_DOUBLE_EQ(exp, act) << code;
        } else {
            ASSERT_EQ(exp, act) << code;
        }
    }, exp);
}

TEST_P(TestEval, testIntrinsic) {
    std::string code("var a : ");
    code = code + GetParam().first + ";";
    testEval(code.c_str(), GetParam().second, true, false);
}

TEST_P(TestEval, testNonIntrinsic) {
    std::string code("var a : ");
    code = code + GetParam().first + ";";
    testEval(code.c_str(), GetParam().second, false, false);
}

TEST_P(TestEval, testConstFolded) {
    std::string code("var a : ");
    code = code + GetParam().first + ";";
    testEval(code.c_str(), GetParam().second, true, true);
}

constexpr int64_t operator"" _i64(unsigned long long in) {
    return static_cast<int64_t>(in);
}

std::string operator"" _s(const char *in, unsigned long len) {
    return {in, len};
}

static TestEvalT evals[] = {
    // Bool arithmetics
    {"Bool = true & true", true},
    {"Bool = true & false", false},
    {"Bool = false & true", false},
    {"Bool = false & false", false},
    {"Bool = true | true", true},
    {"Bool = true | false", true},
    {"Bool = false | true", true},
    {"Bool = false | false", false},
    {"Bool = true ^ true", false},
    {"Bool = true ^ false", true},
    {"Bool = false ^ true", true},
    {"Bool = false ^ false", false},
    // Integer arithmetics
    {"Integer = 1 + 2", 3_i64},
    {"Integer = (10 + 6) + (7 + 3)", 26_i64},
    {"Integer = 100 * 11", 1100_i64},
    {"Integer = 1 * 2 * 3 * 4 * 5 * 6", 720_i64},
    {"Integer = 4 / 3", 1_i64},
    {"Integer = (0-4) / 3", -1_i64},
    {"Integer = 29 / 9", 3_i64},
    {"Integer = 10 % 3", 1_i64},
    {"Integer = 10 % 2", 0_i64},
    {"Integer = 10 % (0-2)", 0_i64},
    {"Integer = 5 % 100", 5_i64},
    {"Integer = -5", -5_i64},
    {"Integer = -5 + 2", -3_i64},
    {"Integer = --10", 10_i64},
    {"Integer = 1 & 3", 1_i64},
    {"Integer = 1 & 2", 0_i64},
    {"Integer = -1 & 2", 2_i64},
    {"Integer = 1 | 3", 3_i64},
    {"Integer = 1 | 2", 3_i64},
    {"Integer = -1 | 2", -1_i64},
    {"Integer = 1 ^ 3", 2_i64},
    {"Integer = 1 ^ 2", 3_i64},
    {"Integer = -1 ^ 2", -3_i64},
    {"Integer = 1 shl 1", 2_i64},
    {"Integer = 1 shl 10", 1_i64 << 10},
    {"Integer = 123 shl 0", 123_i64},
    {"Integer = 123 shrz 0", 123_i64},
    {"Integer = 123 shl 7", 123_i64 << 7},
    {"Integer = 123 shrz 2", 123_i64 >> 2},
    {"Integer = 123 shrz 63", 0_i64},
    {"Integer = 120 shl 63", 0_i64},
    {"Integer = -1 shl 2", -4_i64},
    {"Integer = -4 shrs 2", -1_i64},
    {"Integer = -4 shrz 62", 3_i64},
    {"Integer = 120 shl 1 shl 2", 120_i64 << 3},
    // Float arithmetics
    {"Float = 1.1 + 2.2", 3.3},
    {"Float = 1.1 - 2.2", -1.1},
    {"Float = 1.1 * 2.5", 2.75},
    {"Float = 1e300 * 1e300", std::numeric_limits<double>::infinity()},
    {"Float = 12.3 / 4.56", 12.3 / 4.56},
    {"Float = 12.3 / 0.0", std::numeric_limits<double>::infinity()},
    {"Float = -1.234", -1.234},
    {"Float = --1.234", 1.234},
    // Bool comparisons
    {"Bool = true == true", true},
    {"Bool = true == false", false},
    {"Bool = false == true", false},
    {"Bool = false == false", true},
    {"Bool = true != true", false},
    {"Bool = true != false", true},
    {"Bool = false != true", true},
    {"Bool = false != false", false},
    // Integer comparisons
    {"Bool = 1 == 1 + 2", false},
    {"Bool = 1 + 2 == 1 + 1 + 1", true},
    {"Bool = 1 != 1 + 2", true},
    {"Bool = 1 + 2 != 1 + 1 + 1", false},
    {"Bool = 1 < 2", true},
    {"Bool = 2 < 1", false},
    {"Bool = 1 < 1", false},
    {"Bool = 1 > 2", false},
    {"Bool = 2 > 1", true},
    {"Bool = 1 > 1", false},
    {"Bool = 1 >= 2", false},
    {"Bool = 2 >= 1", true},
    {"Bool = 2 >= 2", true},
    {"Bool = 1 <= 2", true},
    {"Bool = 2 <= 1", false},
    {"Bool = 2 <= 2", true},
    // Float comparisons
    {"Bool = 1.2345 == 1.2345", true},
    {"Bool = 1.2345 == 1.2344", false},
    {"Bool = 1.2345 != 1.2345", false},
    {"Bool = 1.2345 != 1.2344", true},
    {"Bool = 1.2345 < 1.2344", false},
    {"Bool = 1.2344 < 1.2345", true},
    {"Bool = 1.2345 < 1.2345", false},
    {"Bool = 1.2345 > 1.2344", true},
    {"Bool = 1.2344 > 1.2345", false},
    {"Bool = 1.2345 > 1.2345", false},
    {"Bool = 1.2345 <= 1.2344", false},
    {"Bool = 1.2344 <= 1.2345", true},
    {"Bool = 1.2345 <= 1.2345", true},
    {"Bool = 1.2345 >= 1.2344", true},
    {"Bool = 1.2344 >= 1.2345", false},
    {"Bool = 1.2345 <= 1.2345", true},
    // String operations
    {"String = \"Testing memory management for a long string\"", "Testing memory management for a long string"_s},
    {"String = substr(\"Hello World!\", 6, 5)", "World"_s},
    {"String = substr(\"A long string not fitting into the std::string buffer\", 0, 100)",
     "A long string not fitting into the std::string buffer"_s},
    {R"(String = if(true, if (false, "test", substr("A long string not fitting into the std::string buffer", 0, 100)), "test"))",
     "A long string not fitting into the std::string buffer"_s},
};

INSTANTIATE_TEST_SUITE_P(SuiteBuiltins,
                         TestEval,
                         testing::ValuesIn(evals));

} // namespace jex
