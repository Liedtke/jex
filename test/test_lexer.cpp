#include <jex_lexer.hpp>

#include <gtest/gtest.h>

namespace jex {

TEST(Lexer, eof) {
    Token token = Lexer("").getNext();
    EXPECT_EQ(Token::Kind::Eof, token.kind);
    EXPECT_EQ((Location{1, 1, 1, 1}), token.location);
    EXPECT_EQ("", token.text);
}

TEST(Lexer, spaces) {
    Token token = Lexer(" \t\r\n   ").getNext();
    EXPECT_EQ(Token::Kind::Eof, token.kind);
    EXPECT_EQ((Location{2, 4, 2, 4}), token.location);
    EXPECT_EQ("", token.text);
}

TEST(Lexer, invalid) {
    Token token = Lexer(" @").getNext();
    EXPECT_EQ(Token::Kind::Invalid, token.kind);
    EXPECT_EQ((Location{1, 2, 1, 2}), token.location);
    EXPECT_EQ("@", token.text);
}

TEST(Lexer, identifiers) {
    Token token = Lexer("  test").getNext();
    EXPECT_EQ(Token::Kind::Ident, token.kind);
    EXPECT_EQ((Location{1, 3, 1, 6}), token.location);
    EXPECT_EQ("test", token.text);
    token = Lexer("t35t_123\n").getNext();
    EXPECT_EQ(Token::Kind::Ident, token.kind);
    EXPECT_EQ((Location{1, 1, 1, 8}), token.location);
    EXPECT_EQ("t35t_123", token.text);
}

} // namespace jex
