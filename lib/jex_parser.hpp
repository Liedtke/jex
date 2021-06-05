#pragma once

#include <jex_lexer.hpp>

namespace jex {

class CompileEnv;
class AstLiteralExpr;
class IAstExpression;

class Parser {
    CompileEnv& d_env;
    Lexer d_lexer;
    Token d_currToken;

public:
    Parser(CompileEnv& env, const char* source)
    : d_env(env)
    , d_lexer(source) {
        getNextToken();
    }

    void parse();

private:
    Token& getNextToken();
    IAstExpression* parseExpression();
    AstLiteralExpr* parseLiteralInt();
};

} // namespace jex
