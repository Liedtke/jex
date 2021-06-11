#include <jex_compileenv.hpp>
#include <jex_errorhandling.hpp>
#include <jex_parser.hpp>
#include <jex_prettyprinter.hpp>

#include <gtest/gtest.h>

namespace jex {

struct TestExp {
    const char* in;
    const char* exp;
};

class TestParserError : public testing::TestWithParam<TestExp> {};

TEST_P(TestParserError, test) {
    CompileEnv env;
    Parser parser(env, GetParam().in);
    try {
        parser.parse();
        ASSERT_TRUE(false) << "expected CompileError";
    } catch (CompileError& err) {
        ASSERT_STREQ(GetParam().exp, err.what());
    }
}

static TestExp errorTests[] = {
    {"", "1.1-1.1: Unexpected end of file, expecting literal"},
    {"9223372036854775808", "1.1-1.19: Invalid integer literal"},
    {"92233720368547758070", "1.1-1.20: Invalid integer literal"},
};

INSTANTIATE_TEST_SUITE_P(SuiteParserError,
                        TestParserError,
                        testing::ValuesIn(errorTests));

class TestParserSuccess : public testing::TestWithParam<TestExp> {};

TEST_P(TestParserSuccess, test) {
    CompileEnv env;
    Parser parser(env, GetParam().in);
    parser.parse(); // no error
    ASSERT_EQ(0, env.messages().size());
    // check generated ast
    std::stringstream str;
    PrettyPrinter printer(str);
    env.getRoot()->accept(printer);
    ASSERT_EQ(GetParam().exp, str.str());
}

static TestExp successTests[] = {
    {"1", "1"},
    {"123", "123"},
    {"9223372036854775807", "9223372036854775807"},
    {"1+1", "(1 + 1)"},
    {"10 + 11 + 12", "((10 + 11) + 12)"},
    {"1+2*3*4+5", "((1 + ((2 * 3) * 4)) + 5)"}
};

INSTANTIATE_TEST_SUITE_P(SuiteParserSuccess,
                        TestParserSuccess,
                        testing::ValuesIn(successTests));

} // namespace jex
