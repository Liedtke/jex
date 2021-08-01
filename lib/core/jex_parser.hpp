#pragma once

#include <jex_base.hpp>
#include <jex_lexer.hpp>

#include <unordered_map>

namespace jex {

class CompileEnv;
class AstLiteralExpr;
class IAstExpression;
class AstArgList;

class Parser : NoCopy {
    CompileEnv& d_env;
    Lexer d_lexer;
    Token d_currToken;
    std::unordered_map<Token::Kind, int> d_precs;
public:
    Parser(CompileEnv& env, const char* source)
    : d_env(env)
    , d_lexer(env, source) {
        initPrecs();
        getNextToken();
    }

    void parse();

private:
    void initPrecs();
    int getPrec() const;
    Token& getNextToken();
    IAstExpression* parseExpression();
    IAstExpression* parsePrimary();
    IAstExpression* parseBinOpRhs(int prec, IAstExpression* lhs);
    AstLiteralExpr* parseLiteralInt();
    AstLiteralExpr* parseLiteralFloat();
    AstLiteralExpr* parseLiteralString();
    IAstExpression* parseParensExpr();
    IAstExpression* parseIdentOrCall();
    AstArgList* parseArgList();
};

} // namespace jex