#include <test_base.hpp>

#include <jex_compileenv.hpp>
#include <jex_environment.hpp>
#include <jex_errorhandling.hpp>
#include <jex_parser.hpp>
#include <jex_prettyprinter.hpp>
#include <jex_symboltable.hpp>
#include <jex_ast.hpp>

#include <gtest/gtest.h>

namespace jex {

TEST(PrettyPrinter, unsupportedLiteral) {
    Environment environment;
    CompileEnv env(environment);
    AstLiteralExpr node(Location{}, env.typeSystem().unresolved(), "test");
    std::stringstream err;
    PrettyPrinter prettyPrint(err);
    ASSERT_THROW(node.accept(prettyPrint), CompileError);
}

using TestExp = std::pair<const char*, const char*>;

class TestParserError : public testing::TestWithParam<TestExp> {};

TEST_P(TestParserError, test) {
    Environment environment;
    test::registerBuiltIns(environment);
    CompileEnv env(environment);
    TypeInfoId unresolved = env.typeSystem().unresolved();
    env.symbols().addSymbol(Location(), Symbol::Kind::Variable, "x", unresolved);
    env.symbols().addSymbol(Location(), Symbol::Kind::Function, "f", unresolved);
    env.symbols().addSymbol(Location(), Symbol::Kind::Type, "Type", unresolved);
    Parser parser(env, GetParam().first);
    try {
        parser.parse();
    } catch (CompileError& err) {
        ASSERT_STREQ(GetParam().second, err.what());
    }
    ASSERT_EQ(1, env.messages().size());
    std::stringstream errMsg;
    errMsg << *env.messages().begin();
    EXPECT_EQ(GetParam().second, errMsg.str());
}

static TestExp errorTests[] = {
    // var def
    {"x", "1.1-1.1: Error: Unexpected identifier 'x', expecting 'var' or end of file"},
    {"var ", "1.5-1.5: Error: Unexpected end of file, expecting identifier"},
    {"var a", "1.6-1.6: Error: Unexpected end of file, expecting ':'"},
    {"var a: 1", "1.8-1.8: Error: Unexpected integer literal '1', expecting identifier"},
    {"var a: Type", "1.12-1.12: Error: Unexpected end of file, expecting '='"},
    {"var a: Type = ", "1.15-1.15: Error: Unexpected end of file, expecting literal, identifier or '('"},
    {"var a: Type = 1", "1.16-1.16: Error: Unexpected end of file, expecting ';'"},
    {"var a: x = 1;", "1.8-1.8: Error: Invalid type: 'x' is not a type"},
    {"var a: Type = 1;;", "1.17-1.17: Error: Unexpected ';', expecting 'var' or end of file"},
    // TODO: Treat variables and types as different symbols without collisions?
    {"var Type: Type = 1;", "1.5-1.8: Error: Duplicate identifier 'Type'"},
    // expressions
    {"var a: Type = Type;", "1.15-1.18: Error: Invalid expression: 'Type' is not a variable"},
    {"var a: Type = Type();", "1.15-1.18: Error: Invalid call: 'Type' is not a function"},
    {"var a: Type = undefined;", "1.15-1.23: Error: Unknown identifier 'undefined'"},
    {"var a: Type = undefined();", "1.15-1.23: Error: Unknown identifier 'undefined'"},
    {"var a: Type = 9223372036854775808;", "1.15-1.33: Error: Invalid integer literal"},
    {"var a: Type = 92233720368547758070;", "1.15-1.34: Error: Invalid integer literal"},
    {"var a: Type = (", "1.16-1.16: Error: Unexpected end of file, expecting literal, identifier or '('"},
    {"var a: Type = (1", "1.17-1.17: Error: Unexpected end of file, expecting ')'"},
    {"var a: Type = 1++", "1.17-1.17: Error: Unexpected operator '+', expecting literal, identifier or '('"},
    {"var a: Type = 1+*", "1.17-1.17: Error: Unexpected operator '*', expecting literal, identifier or '('"},
    {"var a: Type = 1*/", "1.17-1.17: Error: Unexpected operator '/', expecting literal, identifier or '('"},
    {"var a: Type = (%", "1.16-1.16: Error: Unexpected operator '%', expecting literal, identifier or '('"},
    {"var a: Type = 1*-", "1.17-1.17: Error: Unexpected operator '-', expecting literal, identifier or '('"},
    {"var a: Type = ()", "1.16-1.16: Error: Unexpected ')', expecting literal, identifier or '('"},
    {"var a: Type = 1(", "1.16-1.16: Error: Unexpected '(', expecting ';'"},
    {"var a: Type = 1 + _", "1.19-1.19: Error: Unexpected invalid token '_', expecting literal, identifier or '('"},
    {"var a: Type = 1,", "1.16-1.16: Error: Unexpected ',', expecting ';'"},
    {"var a: Type = 1f", "1.16-1.16: Error: Unexpected identifier 'f', expecting ';'"},
    {"var a: Type = 1..1", "1.17-1.17: Error: Unexpected invalid token '.', expecting ';'"},
    {"var a: Type = 1.e", "1.15-1.17: Error: Invalid floating point literal"},
    {"var a: Type = .1", "1.15-1.15: Error: Unexpected invalid token '.', expecting literal, identifier or '('"}, // TODO: Shall this be allowed?
    {"var a: Type = 1e310", "1.15-1.19: Error: Invalid floating point literal"},
    {"var a: Type = 1e+310", "1.15-1.20: Error: Invalid floating point literal"},
    {"var a: Type = 1e-310", "1.15-1.20: Error: Invalid floating point literal"},
    {"var a: Type = f(,2)", "1.17-1.17: Error: Unexpected ',', expecting literal, identifier or '('"},
    {"var a: Type = f(1 23)", "1.19-1.20: Error: Unexpected integer literal '23', expecting ',' or ')'"},
    {"var a: Type = f(1,2,)", "1.21-1.21: Error: Unexpected ')', expecting literal, identifier or '('"},
    {"var a: Type = x();", "1.15-1.15: Error: Invalid call: 'x' is not a function"},
    {"var a: Type = 1 <=> 2;", "1.19-1.19: Error: Unexpected operator '>', expecting literal, identifier or '('"},
    {"var a: Type = 1 === 2;", "1.19-1.19: Error: Unexpected '=', expecting literal, identifier or '('"},
    {"var a: Type = 1 <> 2;", "1.18-1.18: Error: Unexpected operator '>', expecting literal, identifier or '('"},
};

INSTANTIATE_TEST_SUITE_P(SuiteParserError,
                         TestParserError,
                         testing::ValuesIn(errorTests));

class TestParserSuccess : public testing::TestWithParam<TestExp> {};

TEST_P(TestParserSuccess, test) {
    Environment environment;
    test::registerBuiltIns(environment);
    CompileEnv env(environment);
    Parser parser(env, GetParam().first);
    TypeInfoId unresolved = env.typeSystem().unresolved();
    env.symbols().addSymbol(Location(), Symbol::Kind::Variable, "x", unresolved);
    env.symbols().addSymbol(Location(), Symbol::Kind::Function, "f", unresolved);
    env.symbols().addSymbol(Location(), Symbol::Kind::Type, "Type", unresolved);
    parser.parse();
    EXPECT_FALSE(env.hasErrors());
    ASSERT_EQ(0, env.messages().size()) << *env.messages().begin();
    // check generated ast
    std::stringstream str;
    PrettyPrinter printer(str);
    env.getRoot()->accept(printer);
    ASSERT_EQ(GetParam().second, str.str());
}

static TestExp successTests[] = {
    {"", ""},
    {"// just a comment", ""},
    {"var a: Type = 1;",
     "var a: Type = 1;\n"},
    {"var a: Type = 123;",
     "var a: Type = 123;\n"},
    {"var a: Type = 9223372036854775807;",
     "var a: Type = 9223372036854775807;\n"},
    {"var a: Type = 1.23;",
     "var a: Type = 1.23;\n"},
    {"var a: Type = 10.123e-45;",
     "var a: Type = 1.0123e-44;\n"},
    {"var a: Type = 1e0;",
     "var a: Type = 1;\n"},
    {"var a: Type = 1e1;",
     "var a: Type = 10;\n"},
    {"var a: Type = 11;",
     "var a: Type = 11;\n"},
    {"var a: Type = 1.23e4;",
     "var a: Type = 12300;\n"},
    {"var a: Type = 1+1;",
     "var a: Type = (1 + 1);\n"},
    {"var a: Type = 10 + 11 + 12;",
     "var a: Type = ((10 + 11) + 12);\n"},
    {"var a: Type = 1+2*3*4+5;",
     "var a: Type = ((1 + ((2 * 3) * 4)) + 5);\n"},
    {"var a: Type = 1+1-2*3/4%2;",
     "var a: Type = ((1 + 1) - (((2 * 3) / 4) % 2));\n"},
    {"var a: Type = ((1));",
     "var a: Type = 1;\n"},
    {"var a: Type = ((1) + (1+2));",
     "var a: Type = (1 + (1 + 2));\n"},
    {"var a: Type = 2 * (1 + 3);",
     "var a: Type = (2 * (1 + 3));\n"},
    {"var a: Type = (1 + 3) * (2);",
     "var a: Type = ((1 + 3) * 2);\n"},
    {"var a: Type = x;",
     "var a: Type = x;\n"},
    {"var a: Type = x * 2 + x;",
     "var a: Type = ((x * 2) + x);\n"},
    {"var a: Type = f();",
     "var a: Type = f();\n"},
    {"var a: Type = f(1);",
     "var a: Type = f(1);\n"},
    {"var a: Type = f(1 + 1);",
     "var a: Type = f((1 + 1));\n"},
    {"var a: Type = f(1,x,(2));",
     "var a: Type = f(1, x, 2);\n"},
    {"var a: Type = f(1 + 2, f(f()));",
     "var a: Type = f((1 + 2), f(f()));\n"},
    {"var a: Type = \"\";",
     "var a: Type = \"\";\n"},
    // Note: The pretty printer doesn't escape currently.
    {"var a: Type = \"Hello\\nWorld!\";",
     "var a: Type = \"Hello\nWorld!\";\n"},
    {"var a: Type = 123;\nvar b: Type = a;",
     "var a: Type = 123;\nvar b: Type = a;\n"},
    {"var a: Type = true;",
     "var a: Type = true;\n"},
    {"var a: Type = false;",
     "var a: Type = false;\n"},
    // -- Comparison operators --
    // left-to-right associativity
    {"var a: Type = 1 < 1 < 1;",
     "var a: Type = ((1 < 1) < 1);\n"},
    {"var a: Type = 1 > 1 < 1;",
     "var a: Type = ((1 > 1) < 1);\n"},
    {"var a: Type = 1 <= 1 >= 1;",
     "var a: Type = ((1 <= 1) >= 1);\n"},
    {"var a: Type = 1 == 2 != 3;",
     "var a: Type = ((1 == 2) != 3);\n"},
    // == != have less precedence than < <= > >=
    {"var a: Type = 1 < 2 != 3 > 4;",
     "var a: Type = ((1 < 2) != (3 > 4));\n"},
    {"var a: Type = 1 <= 2 == 3 >= 4;",
     "var a: Type = ((1 <= 2) == (3 >= 4));\n"},
};

INSTANTIATE_TEST_SUITE_P(SuiteParserSuccess,
                         TestParserSuccess,
                         testing::ValuesIn(successTests));

} // namespace jex
