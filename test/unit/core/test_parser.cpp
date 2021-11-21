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
    {"var a: Type =", "1.13-1.13: Error: Unexpected '=', expecting ';'"},
    // expr def
    {"x", "1.1-1.1: Error: Unexpected identifier 'x', expecting 'var', 'const', 'expr' or end of file"},
    {"expr ", "1.6-1.6: Error: Unexpected end of file, expecting identifier"},
    {"expr a", "1.7-1.7: Error: Unexpected end of file, expecting ':'"},
    {"expr a: 1", "1.9-1.9: Error: Unexpected integer literal '1', expecting identifier"},
    {"expr a: Type", "1.13-1.13: Error: Unexpected end of file, expecting '='"},
    {"expr a: Type = ", "1.16-1.16: Error: Unexpected end of file, expecting literal, identifier, '-' or '('"},
    {"expr a: Type = 1", "1.17-1.17: Error: Unexpected end of file, expecting ';'"},
    {"expr a: x = 1;", "1.9-1.9: Error: Invalid type: 'x' is not a type"},
    {"expr a: Type = 1;;", "1.18-1.18: Error: Unexpected ';', expecting 'var', 'const', 'expr' or end of file"},
    // TODO: Treat variables and types as different symbols without collisions?
    {"expr Type: Type = 1;", "1.6-1.9: Error: Duplicate identifier 'Type'"},
    // expressions
    {"expr a: Type = Type;", "1.16-1.19: Error: Invalid expression: 'Type' is not a variable"},
    {"expr a: Type = undefined;", "1.16-1.24: Error: Unknown identifier 'undefined'"},
    {"expr a: Type = undefined();", "1.16-1.24: Error: Unknown identifier 'undefined'"},
    {"expr a: Type = 9223372036854775808;", "1.16-1.34: Error: Invalid integer literal"},
    {"expr a: Type = 92233720368547758070;", "1.16-1.35: Error: Invalid integer literal"},
    {"expr a: Type = (", "1.17-1.17: Error: Unexpected end of file, expecting literal, identifier, '-' or '('"},
    {"expr a: Type = (1", "1.18-1.18: Error: Unexpected end of file, expecting ')'"},
    {"expr a: Type = 1++", "1.18-1.18: Error: Unexpected operator '+', expecting literal, identifier, '-' or '('"},
    {"expr a: Type = 1+*", "1.18-1.18: Error: Unexpected operator '*', expecting literal, identifier, '-' or '('"},
    {"expr a: Type = 1*/", "1.18-1.18: Error: Unexpected operator '/', expecting literal, identifier, '-' or '('"},
    {"expr a: Type = (%", "1.17-1.17: Error: Unexpected operator '%', expecting literal, identifier, '-' or '('"},
    {"expr a: Type = (&", "1.17-1.17: Error: Unexpected operator '&', expecting literal, identifier, '-' or '('"},
    {"expr a: Type = (|", "1.17-1.17: Error: Unexpected operator '|', expecting literal, identifier, '-' or '('"},
    {"expr a: Type = (^", "1.17-1.17: Error: Unexpected operator '^', expecting literal, identifier, '-' or '('"},
    {"expr a: Type = shl", "1.16-1.18: Error: Unexpected operator 'shl', expecting literal, identifier, '-' or '('"},
    {"expr a: Type = shrs", "1.16-1.19: Error: Unexpected operator 'shrs', expecting literal, identifier, '-' or '('"},
    {"expr a: Type = shrz", "1.16-1.19: Error: Unexpected operator 'shrz', expecting literal, identifier, '-' or '('"},
    {"expr a: Type = 1*-", "1.19-1.19: Error: Unexpected end of file, expecting literal, identifier, '-' or '('"},
    {"expr a: Type = ()", "1.17-1.17: Error: Unexpected ')', expecting literal, identifier, '-' or '('"},
    {"expr a: Type = 1(", "1.17-1.17: Error: Unexpected '(', expecting ';'"},
    {"expr a: Type = 1 + _", "1.20-1.20: Error: Unexpected invalid token '_', expecting literal, identifier, '-' or '('"},
    {"expr a: Type = 1,", "1.17-1.17: Error: Unexpected ',', expecting ';'"},
    {"expr a: Type = 1f", "1.17-1.17: Error: Unexpected identifier 'f', expecting ';'"},
    {"expr a: Type = 1..1", "1.18-1.18: Error: Unexpected invalid token '.', expecting ';'"},
    {"expr a: Type = 1.e", "1.16-1.18: Error: Invalid floating point literal"},
    {"expr a: Type = .1", "1.16-1.16: Error: Unexpected invalid token '.', expecting literal, identifier, '-' or '('"}, // TODO: Shall this be allowed?
    {"expr a: Type = 1e310", "1.16-1.20: Error: Invalid floating point literal"},
    {"expr a: Type = 1e+310", "1.16-1.21: Error: Invalid floating point literal"},
    {"expr a: Type = 1e-310", "1.16-1.21: Error: Invalid floating point literal"},
    {"expr a: Type = f(,2)", "1.18-1.18: Error: Unexpected ',', expecting literal, identifier, '-' or '('"},
    {"expr a: Type = f(1 23)", "1.20-1.21: Error: Unexpected integer literal '23', expecting ',' or ')'"},
    {"expr a: Type = f(1,2,)", "1.22-1.22: Error: Unexpected ')', expecting literal, identifier, '-' or '('"},
    {"expr a: Type = x();", "1.16-1.16: Error: Invalid call: 'x' is not a function"},
    {"expr a: Type = 1 <=> 2;", "1.20-1.20: Error: Unexpected operator '>', expecting literal, identifier, '-' or '('"},
    {"expr a: Type = 1 === 2;", "1.20-1.20: Error: Unexpected '=', expecting literal, identifier, '-' or '('"},
    {"expr a: Type = 1 <> 2;", "1.19-1.19: Error: Unexpected operator '>', expecting literal, identifier, '-' or '('"},
    {"expr const:", "1.6-1.10: Error: Unexpected 'const', expecting identifier"},
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
    {"var a: Type;",
     "var a: Type;\n"},
    {"expr a: Type = 1;",
     "expr a: Type = 1;\n"},
    {"expr a: Type = 123;",
     "expr a: Type = 123;\n"},
    {"expr a: Type = 9223372036854775807;",
     "expr a: Type = 9223372036854775807;\n"},
    // Too large for a positive number but fits into a negative value.
    {"expr a: Type = -9223372036854775808;",
     "expr a: Type = -9223372036854775808;\n"},
    {"expr a: Type = 1.23;",
     "expr a: Type = 1.23;\n"},
    {"expr a: Type = 10.123e-45;",
     "expr a: Type = 1.0123e-44;\n"},
    {"expr a: Type = 1e0;",
     "expr a: Type = 1;\n"},
    {"expr a: Type = 1e1;",
     "expr a: Type = 10;\n"},
    {"expr a: Type = 11;",
     "expr a: Type = 11;\n"},
    {"expr a: Type = 1.23e4;",
     "expr a: Type = 12300;\n"},
    {"expr a: Type = 1+1;",
     "expr a: Type = (1 + 1);\n"},
    {"expr a: Type = 10 + 11 + 12;",
     "expr a: Type = ((10 + 11) + 12);\n"},
    {"expr a: Type = 1+2*3*4+5;",
     "expr a: Type = ((1 + ((2 * 3) * 4)) + 5);\n"},
    {"expr a: Type = 1+1-2*3/4%2;",
     "expr a: Type = ((1 + 1) - (((2 * 3) / 4) % 2));\n"},
    {"expr a: Type = ((1));",
     "expr a: Type = 1;\n"},
    {"expr a: Type = ((1) + (1+2));",
     "expr a: Type = (1 + (1 + 2));\n"},
    {"expr a: Type = 2 * (1 + 3);",
     "expr a: Type = (2 * (1 + 3));\n"},
    {"expr a: Type = (1 + 3) * (2);",
     "expr a: Type = ((1 + 3) * 2);\n"},
    {"expr a: Type = x;",
     "expr a: Type = x;\n"},
    {"expr a: Type = x * 2 + x;",
     "expr a: Type = ((x * 2) + x);\n"},
    {"expr a: Type = f();",
     "expr a: Type = f();\n"},
    {"expr a: Type = f(1);",
     "expr a: Type = f(1);\n"},
    {"expr a: Type = f(1 + 1);",
     "expr a: Type = f((1 + 1));\n"},
    {"expr a: Type = f(1,x,(2));",
     "expr a: Type = f(1, x, 2);\n"},
    {"expr a: Type = f(1 + 2, f(f()));",
     "expr a: Type = f((1 + 2), f(f()));\n"},
    {"expr a: Type = \"\";",
     "expr a: Type = \"\";\n"},
    // Note: The pretty printer doesn't escape currently.
    {R"(expr a: Type = "Hello\nWorld!";)",
     "expr a: Type = \"Hello\nWorld!\";\n"},
    {"expr a: Type = 123;\nexpr b: Type = a;",
     "expr a: Type = 123;\nexpr b: Type = a;\n"},
    {"expr a: Type = true;",
     "expr a: Type = true;\n"},
    {"expr a: Type = false;",
     "expr a: Type = false;\n"},
    // -- Comparison operators --
    // left-to-right associativity
    {"expr a: Type = 1 < 1 < 1;",
     "expr a: Type = ((1 < 1) < 1);\n"},
    {"expr a: Type = 1 > 1 < 1;",
     "expr a: Type = ((1 > 1) < 1);\n"},
    {"expr a: Type = 1 <= 1 >= 1;",
     "expr a: Type = ((1 <= 1) >= 1);\n"},
    {"expr a: Type = 1 == 2 != 3;",
     "expr a: Type = ((1 == 2) != 3);\n"},
    // == != have less precedence than < <= > >=
    {"expr a: Type = 1 < 2 != 3 > 4;",
     "expr a: Type = ((1 < 2) != (3 > 4));\n"},
    {"expr a: Type = 1 <= 2 == 3 >= 4;",
     "expr a: Type = ((1 <= 2) == (3 >= 4));\n"},
    // Special if function.
    {"expr a: Type = if(true, 1, 2);",
     "expr a: Type = if(true, 1, 2);\n"},
    // unary minus
    {"expr a: Type = -1;",
     "expr a: Type = -1;\n"},
    {"expr a: Type = --1;",
     "expr a: Type = --1;\n"},
    {"expr a: Type = 1 + -1;",
     "expr a: Type = (1 + -1);\n"},
    {"expr a: Type = 1 + (-1);",
     "expr a: Type = (1 + -1);\n"},
    {"expr a: Type = -1 + 2;",
     "expr a: Type = (-1 + 2);\n"},
    {"expr a: Type = -x;",
     "expr a: Type = -x;\n"},
    {"expr a: Type = -if(true, 1, 2);",
     "expr a: Type = -if(true, 1, 2);\n"},
    // Bitwise operators
    {"expr a: Type = 1 | 2 ^ 3 & 4;",
     "expr a: Type = (1 | (2 ^ (3 & 4)));\n"},
    {"expr a: Type = 1 & 2 | 3 ^ 4;",
     "expr a: Type = ((1 & 2) | (3 ^ 4));\n"},
    {"expr a: Type = 1 & 2 + 3;",
     "expr a: Type = (1 & (2 + 3));\n"},
    {"expr a: Type = 1 & 2 == 3;",
     "expr a: Type = (1 & (2 == 3));\n"},
    // Shift operators
    {"expr a: Type = 1 shl 2 shrz 3 shrs 4;",
     "expr a: Type = (((1 shl 2) shrz 3) shrs 4);\n"},
    {"expr a: Type = 1 shl 2 + 3;",
     "expr a: Type = (1 shl (2 + 3));\n"},
    {"expr a: Type = 1 shl 2 == 3;",
     "expr a: Type = ((1 shl 2) == 3);\n"},
    // Logic operators
    {"expr a: Type = !true;",
     "expr a: Type = !true;\n"},
    {"expr a: Type = true && false;",
     "expr a: Type = (true && false);\n"},
    {"expr a: Type = true && false || false && true;",
     "expr a: Type = ((true && false) || (false && true));\n"},
    {"expr a: Type = true && false && true;",
     "expr a: Type = ((true && false) && true);\n"},
};

INSTANTIATE_TEST_SUITE_P(SuiteParserSuccess,
                         TestParserSuccess,
                         testing::ValuesIn(successTests));

} // namespace jex
