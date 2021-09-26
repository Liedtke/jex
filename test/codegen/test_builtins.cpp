#include <jex_builtins.hpp>

#include <test_base.hpp>

#include <gtest/gtest.h>

#include <limits>
#include <variant>

namespace jex {

using EvalVariant = std::variant<int64_t, double, bool>;
using TestEvalT = std::pair<const char*, EvalVariant>;
class TestEval : public testing::TestWithParam<TestEvalT> {};

static void testEval(const char *code, EvalVariant exp, bool useIntrinsics) {
    Environment env;
    env.addModule(BuiltInsModule());
    CompileResult compiled = compile(env, code, OptLevel::O2, useIntrinsics);
    auto ctx = std::make_unique<char[]>(compiled.getContextSize());
    // Evaluate a.
    const uintptr_t fctAddr = compiled.getFctPtr("a");
    ASSERT_NE(0, fctAddr);
    auto fctA = reinterpret_cast<void* (*)(char*)>(fctAddr);
    std::visit([&](auto exp) {
        auto act = *reinterpret_cast<decltype(exp)*>(fctA(ctx.get()));
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
    testEval(code.c_str(), GetParam().second, true);
}

TEST_P(TestEval, testNonIntrinsic) {
    std::string code("var a : ");
    code = code + GetParam().first + ";";
    testEval(code.c_str(), GetParam().second, false);
}

constexpr int64_t operator"" _i64(unsigned long long in) {
    return static_cast<int64_t>(in);
}

static TestEvalT evals[] = {
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
    // Float arithmetics
    {"Float = 1.1 + 2.2", 3.3},
    {"Float = 1.1 - 2.2", -1.1},
    {"Float = 1.1 * 2.5", 2.75},
    {"Float = 1e300 * 1e300", std::numeric_limits<double>::infinity()},
    {"Float = 12.3 / 4.56", 12.3 / 4.56},
    {"Float = 12.3 / 0.0", std::numeric_limits<double>::infinity()},
    // Bool comarisons
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
};

INSTANTIATE_TEST_SUITE_P(SuiteEval,
                         TestEval,
                         testing::ValuesIn(evals));

} // namespace jex
