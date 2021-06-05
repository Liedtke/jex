#include <jex_lexer.hpp>

#include <algorithm>
#include <cctype>
#include <iostream>

namespace jex {

std::ostream& operator<<(std::ostream& str, const Location& loc) {
    return str << loc.lineBegin << '.' << loc.colBegin << '-' << loc.lineEnd << '.' << loc.colEnd;
}

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

    // parse one character tokens
    switch(*d_cursor) {
        case '\0':
            return setToken(Token::Kind::Eof);
        case '(':
            advance();
            return setToken(Token::Kind::ParensL);
        case ')':
            advance();
            return setToken(Token::Kind::ParensR);
        case ',':
            advance();
            return setToken(Token::Kind::Comma);
        case '+':
            advance();
            return setToken(Token::Kind::OpAdd);
        case '*':
            advance();
            return setToken(Token::Kind::OpMul);
    }

    // parse numeric literals
    if (std::isdigit(*d_cursor)) {
        while (std::isdigit(advance())) {
        }
        return setToken(Token::Kind::LiteralInt);
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
