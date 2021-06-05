#pragma once

#include <jex_location.hpp>

#include <cassert>
#include <iosfwd>
#include <string_view>

namespace jex {

class Token {
public:
    enum class Kind {
        Invalid,
        Eof,
        Ident,
        LiteralInt,
        ParensL,
        ParensR,
        OpAdd,
        OpMul,
        Comma,
    } kind = Kind::Invalid;
    Location location;
    std::string_view text;
};

std::ostream& operator<<(std::ostream& str, const Token& token);

class Lexer {
    const char *d_source;
    const char *d_cursor;
    const char *d_tokenBegin;
    Token d_currToken;
    int d_line = 1;
    int d_col = 1;

public:
    Lexer(const char* source)
    : d_source(source)
    , d_cursor(source)
    , d_tokenBegin(source)
    , d_currToken() {
        assert(d_source != nullptr);
    }

    Token getNext();
    Token setToken(Token::Kind kind);

private:
    char advance();
    void skipWhiteSpaces();
    void resetToken();
};

} // namespace jex
