#pragma once

#include <cassert>
#include <iosfwd>
#include <string_view>

namespace jex {

struct Location {
    int lineBegin = 1;
    int colBegin = 1;
    int lineEnd = 1;
    int colEnd = 1;

    // TODO: use spaceship operator instead
    bool operator==(const Location& other) const {
        return lineBegin == other.lineBegin
            && colBegin == other.colBegin
            && lineEnd == other.lineEnd
            && colEnd == other.colEnd;
    }
};

std::ostream& operator<<(std::ostream& str, const Location& loc);

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
