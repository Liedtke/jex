#include <jex_compileenv.hpp>
#include <jex_errorhandling.hpp>
#include <jex_parser.hpp>
#include <jex_prettyprinter.hpp>
#include <jex_symboltable.hpp>
#include <jex_ast.hpp>

#include <gtest/gtest.h>

namespace jex {

TEST(PrettyPrinter, unsupportedLiteral) {
    AstLiteralExpr node(Location{});
    node.d_resultType = Type::Unresolved;
    PrettyPrinter prettyPrint(std::cerr);
    ASSERT_THROW(node.accept(prettyPrint), CompileError);
}

using TestExp = std::pair<const char*, const char*>;

class TestParserError : public testing::TestWithParam<TestExp> {};

TEST_P(TestParserError, test) {
    CompileEnv env;
    Parser parser(env, GetParam().first);
    try {
        parser.parse();
        ASSERT_TRUE(false) << "expected CompileError"; // LCOV_EXCL_LINE
    } catch (CompileError& err) {
        ASSERT_STREQ(GetParam().second, err.what());
    }
}

static TestExp errorTests[] = {
    {"", "1.1-1.1: Error: Unexpected end of file, expecting literal, identifier or '('"},
    {"9223372036854775808", "1.1-1.19: Error: Invalid integer literal"},
    {"92233720368547758070", "1.1-1.20: Error: Invalid integer literal"},
    {"(", "1.2-1.2: Error: Unexpected end of file, expecting literal, identifier or '('"},
    {"(1", "1.3-1.3: Error: Unexpected end of file, expecting ')'"},
    {"1++", "1.3-1.3: Error: Unexpected operator '+', expecting literal, identifier or '('"},
    {"1+*", "1.3-1.3: Error: Unexpected operator '*', expecting literal, identifier or '('"},
    {"1*/", "1.3-1.3: Error: Unexpected operator '/', expecting literal, identifier or '('"},
    {"(%", "1.2-1.2: Error: Unexpected operator '%', expecting literal, identifier or '('"},
    {"1*-", "1.3-1.3: Error: Unexpected operator '-', expecting literal, identifier or '('"},
    {"()", "1.2-1.2: Error: Unexpected ')', expecting literal, identifier or '('"},
    {"1(", "1.2-1.2: Error: Unexpected '(', expecting an operator or end of file"},
    {"1 + _", "1.5-1.5: Error: Unexpected invalid token '_', expecting literal, identifier or '('"},
    {"1,", "1.2-1.2: Error: Unexpected ',', expecting an operator or end of file"},
    {"1a", "1.2-1.2: Error: Unexpected identifier 'a', expecting an operator or end of file"},
    {"1..1", "1.3-1.3: Error: Unexpected invalid token '.', expecting an operator or end of file"},
    {"1.e", "1.1-1.3: Error: Invalid floating point literal"},
    {".1", "1.1-1.1: Error: Unexpected invalid token '.', expecting literal, identifier or '('"}, // TODO: Shall this be allowed?
    {"1e310", "1.1-1.5: Error: Invalid floating point literal"},
    {"1e+310", "1.1-1.6: Error: Invalid floating point literal"},
    {"1e-310", "1.1-1.6: Error: Invalid floating point literal"},
    {"a(,2)", "1.3-1.3: Error: Unexpected ',', expecting literal, identifier or '('"},
    {"a(1 23)", "1.5-1.6: Error: Unexpected integer literal '23', expecting ',' or ')'"},
    {"a(1,2,)", "1.7-1.7: Error: Unexpected ')', expecting literal, identifier or '('"},
};

INSTANTIATE_TEST_CASE_P(SuiteParserError,
                        TestParserError,
                        testing::ValuesIn(errorTests));

class TestParserSuccess : public testing::TestWithParam<TestExp> {};

TEST_P(TestParserSuccess, test) {
    CompileEnv env;
    Parser parser(env, GetParam().first);
    env.symbols().addSymbol(Location(), Symbol::Kind::Variable, "x");
    env.symbols().addSymbol(Location(), Symbol::Kind::Function, "f");
    parser.parse(); // no error
    ASSERT_EQ(0, env.messages().size()) << *env.messages().begin();
    // check generated ast
    std::stringstream str;
    PrettyPrinter printer(str);
    env.getRoot()->accept(printer);
    ASSERT_EQ(GetParam().second, str.str());
}

static TestExp successTests[] = {
    {"1", "1"},
    {"123", "123"},
    {"9223372036854775807", "9223372036854775807"},
    {"1.23", "1.23"},
    {"10.123e-45", "1.0123e-44"},
    {"1e0", "1"},
    {"1e1", "10"},
    {"11", "11"},
    {"1.23e4", "12300"},
    {"1+1", "(1 + 1)"},
    {"10 + 11 + 12", "((10 + 11) + 12)"},
    {"1+2*3*4+5", "((1 + ((2 * 3) * 4)) + 5)"},
    {"1+1-2*3/4%2", "((1 + 1) - (((2 * 3) / 4) % 2))"},
    {"((1))", "1"},
    {"((1) + (1+2))", "(1 + (1 + 2))"},
    {"2 * (1 + 3)", "(2 * (1 + 3))"},
    {"(1 + 3) * (2)", "((1 + 3) * 2)"},
    {"x", "x"},
    {"x * 2 + x", "((x * 2) + x)"},
    {"f()", "f()"},
    {"f(1)", "f(1)"},
    {"f(1 + 1)", "f((1 + 1))"},
    {"f(1,x,(2))", "f(1, x, 2)"},
    {"f(1 + 2, f(f()))", "f((1 + 2), f(f()))"},
    {"\"\"", "\"\""},
    // Note: The pretty printer doesn't escape currently.
    {"\"Hello\\nWorld!\"", "\"Hello\nWorld!\""},
};

INSTANTIATE_TEST_CASE_P(SuiteParserSuccess,
                        TestParserSuccess,
                        testing::ValuesIn(successTests));

} // namespace jex
