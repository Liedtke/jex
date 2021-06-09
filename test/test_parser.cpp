#include <jex_compileenv.hpp>
#include <jex_errorhandling.hpp>
#include <jex_parser.hpp>

#include <gtest/gtest.h>

namespace jex {

struct ErrorTest {
    const char* in;
    const char* exp;
};

class TestParserError : public testing::TestWithParam<ErrorTest> {};

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

static ErrorTest errorTests[] = {
    {"", "1.1-1.1: Unexpected end of file, expecting literal"},
    {"9223372036854775808", "1.1-1.19: Invalid integer literal"},
    {"92233720368547758070", "1.1-1.20: Invalid integer literal"},
};

INSTANTIATE_TEST_SUITE_P(SuiteParserError,
                        TestParserError,
                        testing::ValuesIn(errorTests));

class TestParserSuccess : public testing::TestWithParam<const char*> {};

TEST_P(TestParserSuccess, test) {
    CompileEnv env;
    Parser parser(env, GetParam());
    parser.parse(); // no error
    ASSERT_EQ(0, env.messages().size());
    // TODO: Check AST
}

static const char* successTests[] = {
    "1",
    "123",
    "9223372036854775807"
};

INSTANTIATE_TEST_SUITE_P(SuiteParserSuccess,
                        TestParserSuccess,
                        testing::ValuesIn(successTests));

} // namespace jex
