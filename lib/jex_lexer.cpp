#include <jex_lexer.hpp>

#include <algorithm>
#include <cctype>

namespace jex {

char Lexer::advance() {
    d_currToken.location.lineEnd = d_line;
    d_currToken.location.colEnd = d_col;

    switch(*d_cursor) {
        case '\n':
            ++d_line;
            d_col = 1;
            break;
        case '\0':
            return '\0'; // Don't do anything
        default:
            d_col += 1;
            break;
    }
    ++d_cursor;
    return *d_cursor;
}

void Lexer::skipWhiteSpaces() {
    while (std::isspace(*d_cursor)) {
        advance();
    }
}

void Lexer::resetToken() {
    d_currToken.location = {d_line, d_col, d_line, d_col};
    d_tokenBegin = d_cursor;
}

Token Lexer::setToken(Token::Kind kind) {
    d_currToken.kind = kind;
    size_t length = d_cursor - d_tokenBegin;
    d_currToken.text = std::string_view(d_tokenBegin, length);
    return d_currToken;
}

Token Lexer::getNext() {
    skipWhiteSpaces();
    resetToken();

    // parse eof
    if (*d_cursor == '\0') {
        return setToken(Token::Kind::Eof);
    }

    // parse identifier: [A-Za-z][A-Za-z0-9_]
    if (std::isalpha(*d_cursor)) {
        while(std::isalnum(*d_cursor) || *d_cursor == '_') {
            advance();
        }
        return setToken(Token::Kind::Ident);
    }

    // invalid: consume single character
    advance();
    return setToken(Token::Kind::Invalid);
}

} // namespace jex
