#include <jex_lexer.hpp>

#include <cctype>
#include <iostream>

namespace jex {

std::ostream& operator<<(std::ostream& str, const Token& token) {
    switch (token.kind) {
        case Token::Kind::Comma:
            return str << "','";
        case Token::Kind::Eof:
            return str << "end of file";
        case Token::Kind::Ident:
            return str << "identifier '" << token.text << '\'';
        case Token::Kind::Invalid:
            return str << "invalid token '" << token.text << "'";
        case Token::Kind::LiteralInt:
            return str << "integer literal '" << token.text << '\'';
        case Token::Kind::LiteralFloat:
            return str << "floating point literal '" << token.text << '\'';
        case Token::Kind::OpAdd:
            return str << "operator '+'";
        case Token::Kind::OpSub:
            return str << "operator '-'";
        case Token::Kind::OpMul:
            return str << "operator '*'";
        case Token::Kind::OpDiv:
            return str << "operator '/'";
        case Token::Kind::OpMod:
            return str << "operator '%'";
        case Token::Kind::ParensL:
            return str << "'('";
        case Token::Kind::ParensR:
            return str << "')'";
    }
    return str;
}

char Lexer::advance() {
    d_currToken.location.end = {d_line, d_col};

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
    d_currToken.location = {{d_line, d_col}, {d_line, d_col}};
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
        case '-':
            advance();
            return setToken(Token::Kind::OpSub);
        case '*':
            advance();
            return setToken(Token::Kind::OpMul);
        case '/':
            advance();
            return setToken(Token::Kind::OpDiv);
        case '%':
            advance();
            return setToken(Token::Kind::OpMod);
    }

    // parse numeric literals
    if (std::isdigit(*d_cursor)) {
        // TODO: Handle hex, binary, octal formats
        while (std::isdigit(advance())) {
        }
        switch(*d_cursor) {
            case '.':
            case 'e':
            case 'E':
                return parseFloatingPoint();
            default:
                return setToken(Token::Kind::LiteralInt);
        }
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

Token Lexer::parseFloatingPoint() {
    // parse fractional digits
    if (*d_cursor == '.') {
        while (std::isdigit(advance())) {
        }
    }
    // parse exponential notation
    if (*d_cursor == 'e' || *d_cursor == 'E') {
        advance();
        if (*d_cursor == '+' || *d_cursor == '-') {
            advance();
        }
        while (std::isdigit(*d_cursor)) {
            advance();
        }
    }
    return setToken(Token::Kind::LiteralFloat);
}

} // namespace jex
