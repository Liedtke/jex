#include <test_base.hpp>

#include <jex_compileenv.hpp>
#include <jex_environment.hpp>
#include <jex_errorhandling.hpp>
#include <jex_lexer.hpp>

#include <gtest/gtest.h>

namespace jex {

TEST(Location, strRep) {
    std::stringstream str;
    str << Location{{1, 2}, {3, 4}};
    EXPECT_EQ("1.2-3.4", str.str());
}

TEST(Location, equals) {
    EXPECT_TRUE((Location{{1, 2}, {3, 4}} == Location{{1, 2}, {3, 4}}));
    EXPECT_FALSE((Location{{1, 2}, {3, 4}} == Location{{0, 2}, {3, 4}}));
    EXPECT_FALSE((Location{{1, 2}, {3, 4}} == Location{{1, 0}, {3, 4}}));
    EXPECT_FALSE((Location{{1, 2}, {3, 4}} == Location{{1, 2}, {0, 4}}));
    EXPECT_FALSE((Location{{1, 2}, {3, 4}} == Location{{1, 2}, {3, 0}}));
}

TEST(Location, defaultConstruction) {
    Location loc;
    EXPECT_EQ(1, loc.begin.line);
    EXPECT_EQ(1, loc.begin.col);
    EXPECT_EQ(1, loc.begin.line);
    EXPECT_EQ(1, loc.end.col);
}

TEST(Lexer, functionCall) {
    Environment environment;
    CompileEnv env(environment);
    Lexer lexer(env, "fct(1, 23)");
    Token token = lexer.getNext();
    EXPECT_EQ(Token::Kind::Ident, token.kind);
    EXPECT_EQ((Location{{1, 1}, {1, 3}}), token.location);
    EXPECT_EQ("fct", token.text);
    token = lexer.getNext();
    EXPECT_EQ(Token::Kind::ParensL, token.kind);
    EXPECT_EQ((Location{{1, 4}, {1, 4}}), token.location);
    EXPECT_EQ("(", token.text);
    token = lexer.getNext();
    EXPECT_EQ(Token::Kind::LiteralInt, token.kind);
    EXPECT_EQ((Location{{1, 5}, {1, 5}}), token.location);
    EXPECT_EQ("1", token.text);
    token = lexer.getNext();
    EXPECT_EQ(Token::Kind::Comma, token.kind);
    EXPECT_EQ((Location{{1, 6}, {1, 6}}), token.location);
    EXPECT_EQ(",", token.text);
    token = lexer.getNext();
    EXPECT_EQ(Token::Kind::LiteralInt, token.kind);
    EXPECT_EQ((Location{{1, 8}, {1, 9}}), token.location);
    EXPECT_EQ("23", token.text);
    token = lexer.getNext();
    EXPECT_EQ(Token::Kind::ParensR, token.kind);
    EXPECT_EQ((Location{{1, 10}, {1, 10}}), token.location);
    EXPECT_EQ(")", token.text);
}

TEST(Lexer, eofGetNext) {
    // test calling next when eof already reached.
    Environment environment;
    CompileEnv env(environment);
    Lexer lexer(env, "");
    Token token = lexer.getNext();
    EXPECT_EQ(Token::Kind::Eof, token.kind);
    EXPECT_EQ((Location{{1, 1}, {1, 1}}), token.location);
    EXPECT_EQ("", token.text);
    token = lexer.getNext();
    EXPECT_EQ(Token::Kind::Eof, token.kind);
    EXPECT_EQ((Location{{1, 1}, {1, 1}}), token.location);
    EXPECT_EQ("", token.text);
}

using TestSingleToken = std::pair<const char*, Token>;

class TestToken : public testing::TestWithParam<TestSingleToken> {};

static TestSingleToken tokenTests[] = {
    {"", Token{Token::Kind::Eof, Location{{1, 1}, {1, 1}}, ""}},
    {" \t\r\n   ", Token{Token::Kind::Eof, Location{{2, 4}, {2, 4}}, ""}},
    {" @ ", Token{Token::Kind::Invalid, Location{{1, 2}, {1, 2}}, "@"}},
    {" + ", Token{Token::Kind::OpAdd, Location{{1, 2}, {1, 2}}, "+"}},
    {" - ", Token{Token::Kind::OpSub, Location{{1, 2}, {1, 2}}, "-"}},
    {" * ", Token{Token::Kind::OpMul, Location{{1, 2}, {1, 2}}, "*"}},
    {" / ", Token{Token::Kind::OpDiv, Location{{1, 2}, {1, 2}}, "/"}},
    {" % ", Token{Token::Kind::OpMod, Location{{1, 2}, {1, 2}}, "%"}},
    {" <>", Token{Token::Kind::OpLT, Location{{1, 2}, {1, 2}}, "<"}},
    {" > ", Token{Token::Kind::OpGT, Location{{1, 2}, {1, 2}}, ">"}},
    {" == ", Token{Token::Kind::OpEQ, Location{{1, 2}, {1, 3}}, "=="}},
    {" != ", Token{Token::Kind::OpNE, Location{{1, 2}, {1, 3}}, "!="}},
    {" <==", Token{Token::Kind::OpLE, Location{{1, 2}, {1, 3}}, "<="}},
    {" >==", Token{Token::Kind::OpGE, Location{{1, 2}, {1, 3}}, ">="}},
    {"  test", Token{Token::Kind::Ident, Location{{1, 3}, {1, 6}}, "test"}},
    {"t35t_123\n", Token{Token::Kind::Ident, Location{{1, 1}, {1, 8}}, "t35t_123"}},
    {"1234567", Token{Token::Kind::LiteralInt, Location{{1, 1}, {1, 7}}, "1234567"}},
    {"1.1 ", Token{Token::Kind::LiteralFloat, Location{{1, 1}, {1, 3}}, "1.1"}},
    {"1e3 ", Token{Token::Kind::LiteralFloat, Location{{1, 1}, {1, 3}}, "1e3"}},
    {"11e20 ", Token{Token::Kind::LiteralFloat, Location{{1, 1}, {1, 5}}, "11e20"}},
    {"1.1.1 ", Token{Token::Kind::LiteralFloat, Location{{1, 1}, {1, 3}}, "1.1"}},
    {"1e1.1 ", Token{Token::Kind::LiteralFloat, Location{{1, 1}, {1, 3}}, "1e1"}},
    {"1e10e1 ", Token{Token::Kind::LiteralFloat, Location{{1, 1}, {1, 4}}, "1e10"}},
    {"1.123e10 ", Token{Token::Kind::LiteralFloat, Location{{1, 1}, {1, 8}}, "1.123e10"}},
    {"1.123E-10 ", Token{Token::Kind::LiteralFloat, Location{{1, 1}, {1, 9}}, "1.123E-10"}},
    {"1234567.123e+100 ", Token{Token::Kind::LiteralFloat, Location{{1, 1}, {1, 16}}, "1234567.123e+100"}},
    // The lexer treats everything that is a valid start of a floating point literal as a floating point literal
    // even if the result might be malformed. This way the error message can be clarer, e.g. the parser reporting
    // "invalid floating point literal '1.123e+'".
    {"1.123e+-10 ", Token{Token::Kind::LiteralFloat, Location{{1, 1}, {1, 7}}, "1.123e+"}},
    // test comment parsing
    {"// unterminated line comment", Token{Token::Kind::Eof, Location{{1, 29}, {1, 29}}, ""}},
    {"// unterminated line comment\n123", Token{Token::Kind::LiteralInt, Location{{2, 1}, {2, 3}}, "123"}},
    {"////\n///\n//\n/", Token{Token::Kind::OpDiv, Location{{4, 1}, {4, 1}}, "/"}},
    {"/**/", Token{Token::Kind::Eof, Location{{1, 5}, {1, 5}}, ""}},
    {"/***/", Token{Token::Kind::Eof, Location{{1, 6}, {1, 6}}, ""}},
    {"/* /*/", Token{Token::Kind::Eof, Location{{1, 7}, {1, 7}}, ""}},
    {"/* /**//", Token{Token::Kind::OpDiv, Location{{1, 8}, {1, 8}}, "/"}},
    {"/*Comment\nspanning\nmultiple\nlines\n*/", Token{Token::Kind::Eof, Location{{5, 3}, {5, 3}}, ""}},
    {"/*@$-_\\\t&|^% */", Token{Token::Kind::Eof, Location{{1, 16}, {1, 16}}, ""}},
    // string literals
    {"\"\"", Token{Token::Kind::LiteralString, Location{{1, 1}, {1, 2}}, ""}},
    {"\"Hello!\"", Token{Token::Kind::LiteralString, Location{{1, 1}, {1, 8}}, "Hello!"}},
    {R"("\n\t")", Token{Token::Kind::LiteralString, Location{{1, 1}, {1, 6}}, "\n\t"}},
    {R"jex("'\'\"\?\\\a\b\f\n\r\t\v")jex", Token{Token::Kind::LiteralString, Location{{1, 1}, {1, 25}}, "'\'\"\?\\\a\b\f\n\r\t\v"}},
    {" : ", Token{Token::Kind::Colon, Location{{1, 2}, {1, 2}}, ":"}},
    {" ; ", Token{Token::Kind::Semicolon, Location{{1, 2}, {1, 2}}, ";"}},
    {" = ", Token{Token::Kind::Assign, Location{{1, 2}, {1, 2}}, "="}},
    {" var ", Token{Token::Kind::Var, Location{{1, 2}, {1, 4}}, "var"}},
    {" varx ", Token{Token::Kind::Ident, Location{{1, 2}, {1, 5}}, "varx"}},
    {" true ", Token{Token::Kind::LiteralBool, Location{{1, 2}, {1, 5}}, "true"}},
    {" false ", Token{Token::Kind::LiteralBool, Location{{1, 2}, {1, 6}}, "false"}},
};

INSTANTIATE_TEST_SUITE_P(SuiteTokens,
                         TestToken,
                         testing::ValuesIn(tokenTests));

TEST_P(TestToken, lex) {
    Environment environment;
    CompileEnv env(environment);
    Lexer lexer(env, GetParam().first);
    Token token = lexer.getNext();
    const Token& exp = GetParam().second;
    EXPECT_EQ(exp.kind, token.kind);
    EXPECT_EQ(exp.location, token.location);
    EXPECT_EQ(exp.text, token.text);
}

using TestException = std::pair<const char*, const char*>;
class TestLexerException : public testing::TestWithParam<TestException> {};

static TestException exceptionTests[] = {
    {"/* hello", "1.1-1.8: Error: Unterminated comment"},
    {"/*/", "1.1-1.3: Error: Unterminated comment"},
    {"/** /", "1.1-1.5: Error: Unterminated comment"},
    {R"("t\&")", "1.2-1.3: Error: Invalid escape sequence '\\&'"},
    {"\"hello", "1.1-1.6: Error: Unterminated string literal"},
    {"\"\\", "1.1-1.3: Error: Unterminated string literal"},
};

INSTANTIATE_TEST_SUITE_P(SuiteLexerExceptions,
                         TestLexerException,
                         testing::ValuesIn(exceptionTests));

TEST_P(TestLexerException, lex) {
    Environment environment;
    CompileEnv env(environment);
    Lexer lexer(env, GetParam().first);
    try {
        Token token = lexer.getNext();
        ASSERT_TRUE(false) << "Expected exception, got token " << token; // LCOV_EXCL_LINE
    } catch (const CompileError& e) {
        ASSERT_TRUE(env.hasErrors());
        ASSERT_LT(0, env.messages().size()) << "Exception must also be stored in messages";
        ASSERT_STREQ(e.what(), GetParam().second);
    }
}

} // namespace jex
