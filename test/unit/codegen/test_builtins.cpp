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
    std::string code("expr a : ");
    code = code + GetParam().first + ";";
    testEval(code.c_str(), GetParam().second, true, false);
}

TEST_P(TestEval, testNonIntrinsic) {
    std::string code("expr a : ");
    code = code + GetParam().first + ";";
    testEval(code.c_str(), GetParam().second, false, false);
}

TEST_P(TestEval, testConstFolded) {
    std::string code("expr a : ");
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
    // Integer constructors
    {"Integer = Integer(false)", 0_i64},
    {"Integer = Integer(true)", 1_i64},
    {"Integer = Integer(-1.23)", -1_i64},
    {"Integer = Integer(0.0)", 0_i64},
    {"Integer = Integer(1e10)", 10000000000_i64},
    // Float constructors
    {"Float = Float(false)", 0.0},
    {"Float = Float(true)", 1.0},
    {"Float = Float(12345)", 12345.0},
    {"Float = Float(-12345)", -12345.0},
    {"Float = Float(0)", 0.0},
    // String constructors
    {R"(String = String(true))", "1"_s},
    {R"(String = String(false))", "0"_s},
    {R"(String = String(0))", "0"_s},
    {R"(String = String(1234567))", "1234567"_s},
    {R"(String = String(-987654321))", "-987654321"_s},
    {R"(String = String(1e10))", "10000000000.000000"_s},
    {R"(String = String(-123.456))", "-123.456000"_s},
    {R"(String = String(0.0))", "0.000000"_s},
    {R"(String = String(1.0 / 0.0))", "inf"_s},
    {R"(String = String(-1.0 / 0.0))", "-inf"_s},
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
    {"Bool = !true", false},
    {"Bool = !false", true},
    {"Bool = !!true", true},
    {"Bool = !!false", false},
    {"Bool = true && true", true},
    {"Bool = true && false", false},
    {"Bool = false && true", false},
    {"Bool = false && false", false},
    {"Bool = true || true", true},
    {"Bool = true || false", true},
    {"Bool = false || true", true},
    {"Bool = false || false", false},
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
    {"Integer = 123 shrs 0", 123_i64},
    {"Integer = 123 shl 7", 123_i64 << 7},
    {"Integer = 123 shrz 2", 123_i64 >> 2},
    {"Integer = 123 shrz 63", 0_i64},
    {"Integer = 120 shl 63", 0_i64},
    {"Integer = -1 shl 2", -4_i64},
    {"Integer = -4 shrs 2", -1_i64},
    {"Integer = 4 shrs 2", 1_i64},
    {"Integer = -4 shrz 62", 3_i64},
    {"Integer = 120 shl 1 shl 2", 120_i64 << 3},
    {"Integer = max(1)", 1_i64},
    {"Integer = max(1, 2)", 2_i64},
    {"Integer = max(-1, 42, 11, 27)", 42_i64},
    {"Integer = max(-10, -10-1, -10+1)", -9_i64},
    {"Integer = max(1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20)", 20_i64},
    {"Integer = max(1, max(2, max(3, max(4), 5), 6), 7)", 7_i64},
    // Float arithmetics
    {"Float = 1.1 + 2.2", 3.3},
    {"Float = 1.1 - 2.2", -1.1},
    {"Float = 1.1 * 2.5", 2.75},
    {"Float = 1e300 * 1e300", std::numeric_limits<double>::infinity()},
    {"Float = 12.3 / 4.56", 12.3 / 4.56},
    {"Float = 12.3 / 0.0", std::numeric_limits<double>::infinity()},
    {"Float = -1.234", -1.234},
    {"Float = --1.234", 1.234},
    {"Float = max(-2.0, -1.0, -3.0)", -1.0},
    {"Float = max(0.0, 1.1, 1.1, 0.0)", 1.1},
    {"Float = max(1.0/0.0, 2.0)", std::numeric_limits<double>::infinity()},
    {"Float = max(-1.0/0.0, -2.0)", -2.0},
    {"Float = max(-1.0/0.0, 2.0/0.0)", std::numeric_limits<double>::infinity()},
    {"Float = max(-1.0/0.0, -2.0/0.0)", -std::numeric_limits<double>::infinity()},
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
    // String comparsions
    {R"(Bool = "test" == "test")", true},
    {R"(Bool = "test" == "testit")", false},
    {R"(Bool = "test" != "test")", false},
    {R"(Bool = "test" != "testit")", true},
    {R"(Bool = "a" < "b")", true},
    {R"(Bool = "b" < "a")", false},
    {R"(Bool = "a" < "a")", false},
    {R"(Bool = "a" <= "b")", true},
    {R"(Bool = "b" <= "a")", false},
    {R"(Bool = "a" <= "a")", true},
    {R"(Bool = "a" > "b")", false},
    {R"(Bool = "b" > "a")", true},
    {R"(Bool = "a" > "a")", false},
    {R"(Bool = "a" >= "b")", false},
    {R"(Bool = "b" >= "a")", true},
    {R"(Bool = "a" >= "a")", true},
    // String operations
    {"String = \"Testing memory management for a long string\"", "Testing memory management for a long string"_s},
    {"String = substr(\"Hello World!\", 6, 5)", "World"_s},
    {"String = substr(\"A long string not fitting into the std::string buffer\", 0, 100)",
     "A long string not fitting into the std::string buffer"_s},
    {R"(String = if(true, if (false, "test", substr("A long string not fitting into the std::string buffer", 0, 100)), "test"))",
     "A long string not fitting into the std::string buffer"_s},
    {R"(String = join(" ehm ", "This", "works", "well."))",
     "This ehm works ehm well."_s},
    {R"(String = join("", "First", "Second", "Third", "Fourth", "", "Sixth"))",
     "FirstSecondThirdFourthSixth"_s},
    {R"(String = join("", ""))",
     ""_s},
    {R"(String = join("concatenated", "This", "is", "a", "test"))",
     "Thisconcatenatedisconcatenatedaconcatenatedtest"_s},
    // TODO: Move the following tests to another test file as they don't test the built-ins.
    {R"(Bool = substr("This is a long string not fitting into short string optimization", 0, 100) == "test" ||
        substr("This is a long string not fitting into short string optimization", 0, 100) != "test")", true},
    {R"(Bool = substr("This is a long string not fitting into short string optimization", 0, 100) != "test" ||
        substr("This is a long string not fitting into short string optimization", 0, 100) == "test")", true},
    {R"(Bool = substr("This is a long string not fitting into short string optimization", 0, 100) == "test" &&
        substr("This is a long string not fitting into short string optimization", 0, 100) != "test")", false},
    {R"(Bool = substr("This is a long string not fitting into short string optimization", 0, 100) != "test" &&
        substr("This is a long string not fitting into short string optimization", 0, 100) == "test")", false},
};

INSTANTIATE_TEST_SUITE_P(SuiteBuiltins,
                         TestEval,
                         testing::ValuesIn(evals));

} // namespace jex
