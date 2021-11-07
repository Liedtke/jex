#pragma once

#include <jex_base.hpp>
#include <jex_location.hpp>

#include <cassert>
#include <iosfwd>
#include <string_view>
#include <vector>

namespace jex {

class CompileEnv;

class Token {
public:
    enum class Kind {
        Invalid,
        Eof,
        Ident,
        LiteralBool,
        LiteralInt,
        LiteralFloat,
        LiteralString,
        ParensL,
        ParensR,
        OpAdd,    // +
        OpSub,    // -
        OpMul,    // *
        OpDiv,    // /
        OpMod,    // %
        OpEQ,     // ==
        OpNE,     // !=
        OpLT,     // <
        OpGT,     // >
        OpLE,     // <=
        OpGE,     // >=
        OpBitAnd, // &
        OpBitOr,  // |
        OpBitXor, // ^
        OpNot,    // !
        OpAnd,    // &&
        OpOr,     // ||
        OpShl,    // shl
        OpShrs,   // shrs
        OpShrz,   // shrz
        Comma,
        Colon,
        Semicolon,
        Var,
        Const,
        Assign,
    } kind = Kind::Invalid;
    Location location;
    std::string_view text;
};

std::ostream& operator<<(std::ostream& str, const Token& token);

class Lexer : NoCopy {
    CompileEnv& d_env;
    const char *d_source;
    const char *d_cursor;
    const char *d_tokenBegin;
    Token d_currToken;
    int d_line = 1;
    int d_col = 1;
    std::vector<char> d_strBuffer;

public:
    Lexer(CompileEnv& env, const char* source)
    : d_env(env)
    , d_source(source)
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
    Token parseFloatingPoint();
    Token parseStringLiteral();
    void parseEscapedChar();
    template <class ... Char>
    void advanceUntil(Char ... args);
};

} // namespace jex
