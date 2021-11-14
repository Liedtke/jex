#include <jex_lexer.hpp>

#include <jex_compileenv.hpp>

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
        case Token::Kind::LiteralBool:
            return str << "bool literal '" << token.text << '\'';
        case Token::Kind::LiteralInt:
            return str << "integer literal '" << token.text << '\'';
        case Token::Kind::LiteralFloat:
            return str << "floating point literal '" << token.text << '\'';
        case Token::Kind::LiteralString:
            return str << "string literal '" << token.text << '\'';
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
        case Token::Kind::OpEQ:
            return str << "operator '=='";
        case Token::Kind::OpNE:
            return str << "operator '!='";
        case Token::Kind::OpLT:
            return str << "operator '<'";
        case Token::Kind::OpGT:
            return str << "operator '>'";
        case Token::Kind::OpLE:
            return str << "operator '<='";
        case Token::Kind::OpGE:
            return str << "operator '>='";
        case Token::Kind::OpBitAnd:
            return str << "operator '&'";
        case Token::Kind::OpBitOr:
            return str << "operator '|'";
        case Token::Kind::OpBitXor:
            return str << "operator '^'";
        case Token::Kind::OpNot:
            return str << "operator '!'";
        case Token::Kind::OpAnd:
            return str << "operator '&&'";
        case Token::Kind::OpOr:
            return str << "operator '||'";
        case Token::Kind::OpShl:
            return str << "operator 'shl'";
        case Token::Kind::OpShrs:
            return str << "operator 'shrs'";
        case Token::Kind::OpShrz:
            return str << "operator 'shrz'";
        case Token::Kind::ParensL:
            return str << "'('";
        case Token::Kind::ParensR:
            return str << "')'";
        case Token::Kind::Colon:
            return str << "':'";
        case Token::Kind::Semicolon:
            return str << "';'";
        case Token::Kind::Assign:
            return str << "'='";
        case Token::Kind::Var:
            return str << "'var'";
        case Token::Kind::Const:
            return str << "'const'";
        case Token::Kind::Expr:
            return str << "'expr'";
    }
    return str; // LCOV_EXCL_LINE unreachable
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

template <typename ... Char>
void Lexer::advanceUntil(Char ... args) {
    static_assert((... && std::is_same_v<char, Char>));
    while ((... && (*d_cursor != args))) {
        advance();
    }
}

Token Lexer::getNext() {
    while (true) {
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
            case '"':
                return parseStringLiteral();
            case '/':
                advance();
                // handle line comments //
                if (*d_cursor == '/') {
                    advance(); // consume '/'
                    advanceUntil('\n', '\0');
                    continue;
                }
                // handle block comment /* */
                if (*d_cursor == '*') {
                    advance(); // consume '*;
                    while (true) {
                        advanceUntil('*', '\0');
                        if (*d_cursor == '*' && advance() == '/') {
                            advance(); // consume '/'
                            break;
                        }
                        if (*d_cursor == '\0') {
                            d_env.throwError(d_currToken.location, "Unterminated comment");
                        }
                    }
                    continue; // comment finished
                }
                return setToken(Token::Kind::OpDiv);
            case '%':
                advance();
                return setToken(Token::Kind::OpMod);
            case ':':
                advance();
                return setToken(Token::Kind::Colon);
            case ';':
                advance();
                return setToken(Token::Kind::Semicolon);
            case '=':
                advance();
                if (*d_cursor == '=') {
                    advance(); // consume '='
                    return setToken(Token::Kind::OpEQ);
                }
                return setToken(Token::Kind::Assign);
            case '<':
                advance();
                if (*d_cursor == '=') {
                    advance(); // consume '='
                    return setToken(Token::Kind::OpLE);
                }
                return setToken(Token::Kind::OpLT);
            case '>':
                advance();
                if (*d_cursor == '=') {
                    advance(); // consume '='
                    return setToken(Token::Kind::OpGE);
                }
                return setToken(Token::Kind::OpGT);
            case '!':
                advance();
                if (*d_cursor == '=') {
                    advance(); // consume '='
                    return setToken(Token::Kind::OpNE);
                }
                return setToken(Token::Kind::OpNot);
            case '&':
                advance();
                if (*d_cursor == '&') {
                    advance(); // consume '&'
                    return setToken(Token::Kind::OpAnd);
                }
                return setToken(Token::Kind::OpBitAnd);
            case '|':
                advance();
                if (*d_cursor == '|') {
                    advance(); // consume '|'
                    return setToken(Token::Kind::OpOr);
                }
                return setToken(Token::Kind::OpBitOr);
            case '^':
                advance();
                return setToken(Token::Kind::OpBitXor);
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
            std::string_view text = std::string_view(d_tokenBegin, d_cursor - d_tokenBegin);
            // TODO: use unordered_map or similar for lookup of reserved keywords.
            if (text == "var") {
                return setToken(Token::Kind::Var);
            }
            if (text == "const") {
                return setToken(Token::Kind::Const);
            }
            if (text == "expr") {
                return setToken(Token::Kind::Expr);
            }
            if (text == "true" || text == "false") {
                return setToken(Token::Kind::LiteralBool);
            }
            if (text == "shl") {
                return setToken(Token::Kind::OpShl);
            }
            if (text == "shrz") {
                return setToken(Token::Kind::OpShrz);
            }
            if (text == "shrs") {
                return setToken(Token::Kind::OpShrs);
            }
            return setToken(Token::Kind::Ident);
        }

        // invalid: consume single character
        advance();
        return setToken(Token::Kind::Invalid);
    }
}

Token Lexer::parseStringLiteral() {
    advance(); // consume '"'
    while (true) {
        switch (*d_cursor) {
            case '\0':
                d_env.throwError(d_currToken.location, "Unterminated string literal");
            case '\\':
                parseEscapedChar();
                break;
            case '"':
                advance(); // consume '"'
                d_currToken.kind = Token::Kind::LiteralString;
                d_currToken.text = d_env.createStringLiteral({d_strBuffer.data(), d_strBuffer.size()});
                d_strBuffer.clear();
                return d_currToken;
            default:
                d_strBuffer.push_back(*d_cursor);
                advance();
        }
    }
}

void Lexer::parseEscapedChar() {
    advance(); // consume '\'
    // TODO: add support for numeric and unicode escape sequences
    switch(*d_cursor) {
        case '\\':
            d_strBuffer.push_back('\\');
            break;
        case '\'':
            d_strBuffer.push_back('\'');
            break;
        case '?':
            d_strBuffer.push_back('?');
            break;
        case '"':
            d_strBuffer.push_back('"');
            break;
        case 'a':
            d_strBuffer.push_back('\a');
            break;
        case 'b':
            d_strBuffer.push_back('\b');
            break;
        case 'f':
            d_strBuffer.push_back('\f');
            break;
        case 'n':
            d_strBuffer.push_back('\n');
            break;
        case 'r':
            d_strBuffer.push_back('\r');
            break;
        case 't':
            d_strBuffer.push_back('\t');
            break;
        case 'v':
            d_strBuffer.push_back('\v');
            break;
        case '\0':
            break; // don't do anything, will be handled by caller
        default:
            d_env.throwError(
                {{d_currToken.location.end.line, d_currToken.location.end.col - 1}, d_currToken.location.end},
                std::string("Invalid escape sequence '") + d_cursor[-1] + d_cursor[0] + '\'');
    }
    advance();
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
