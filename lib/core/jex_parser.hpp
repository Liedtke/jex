#pragma once

#include <jex_base.hpp>
#include <jex_lexer.hpp>

#include <unordered_map>

namespace jex {

class CompileEnv;
class AstLiteralExpr;
class IAstExpression;
class AstArgList;
class AstVariableDef;
class AstIdentifier;
class AstRoot;

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
    IAstExpression* parseExpression();

private:
    void initPrecs();
    int getPrec() const;
    Token& getNextToken();
    IAstExpression* parsePrimary();
    IAstExpression* parseUnaryMinus();
    IAstExpression* parseBinOpRhs(int prec, IAstExpression* lhs);
    AstLiteralExpr* parseLiteralBool();
    AstLiteralExpr* parseLiteralInt();
    AstLiteralExpr* parseLiteralFloat();
    AstLiteralExpr* parseLiteralString();
    IAstExpression* parseParensExpr();
    AstIdentifier* parseIdent();
    IAstExpression* parseIdentOrCall();
    AstArgList* parseArgList();
    AstVariableDef* parseVariableDef();
    AstRoot* parseRoot();
    [[noreturn]] void throwUnexpected(std::string_view expecting);
};

} // namespace jex
