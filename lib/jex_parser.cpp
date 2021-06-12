#include <jex_parser.hpp>

#include <jex_ast.hpp>
#include <jex_compileenv.hpp>

#include <sstream>
#include <string>

namespace jex {

namespace {

OpType getOp(const Token& op, CompileEnv& env) {
    switch (op.kind) {
        case Token::Kind::OpAdd:
            return OpType::Add;
        case Token::Kind::OpSub:
            return OpType::Sub;
        case Token::Kind::OpMul:
            return OpType::Mul;
        case Token::Kind::OpDiv:
            return OpType::Div;
        case Token::Kind::OpMod:
            return OpType::Mod;
        default:
            env.throwError(op.location, "Invalid operator '" + std::string(op.text) + "'");
    }
}

} // anonymous namespace

void Parser::initPrecs() {
    // + -
    d_precs[Token::Kind::OpAdd] = 10;
    d_precs[Token::Kind::OpSub] = 10;
    // * / %
    d_precs[Token::Kind::OpMul] = 20;
    d_precs[Token::Kind::OpDiv] = 20;
    d_precs[Token::Kind::OpMod] = 20;
}

int Parser::getPrec() const {
    auto found = d_precs.find(d_currToken.kind);
    return found != d_precs.end() ? found->second : -1;
};

Token& Parser::getNextToken() {
    d_currToken = d_lexer.getNext();
    return d_currToken;
}

void Parser::parse() {
    d_env.setRoot(parseExpression());
}

IAstExpression* Parser::parseParensExpr() {
    assert(d_currToken.kind == Token::Kind::ParensL);
    getNextToken();
    IAstExpression* expr = parseExpression();
    if (d_currToken.kind != Token::Kind::ParensR) {
        std::stringstream msg;
        msg << "Unexpected " << d_currToken << ", expecting ')'";
        d_env.throwError(d_currToken.location, msg.str());
    }
    getNextToken();
    return expr;
}

IAstExpression* Parser::parsePrimary() {
    switch (d_currToken.kind) {
        case Token::Kind::LiteralInt:
            return parseLiteralInt();
        case Token::Kind::ParensL:
            return parseParensExpr();
        default:
            std::stringstream msg;
            // TODO: extend expectation list
            msg << "Unexpected " << d_currToken << ", expecting literal or '('";
            d_env.throwError(d_currToken.location, msg.str());
    }
}

IAstExpression* Parser::parseBinOpRhs(int prec, IAstExpression* lhs) {
    while (true) {
        int tokPrec = getPrec();
        // token has lesser precedence
        if (tokPrec < prec) {
            return lhs;
        }
        Token binOp = d_currToken;
        getNextToken(); // consume binary operator
        IAstExpression* rhs = parsePrimary();

        int nextPrec = getPrec();
        if (tokPrec < nextPrec) {
            rhs = parseBinOpRhs(tokPrec + 1, rhs);
        }
        Location loc = Location::combine(lhs->d_loc, rhs->d_loc);
        lhs = d_env.createNode<AstBinaryExpr>(loc, getOp(binOp, d_env), lhs, rhs);
    }
}

IAstExpression* Parser::parseExpression() {
    IAstExpression* lhs = parsePrimary();
    return parseBinOpRhs(0, lhs);
}

AstLiteralExpr* Parser::parseLiteralInt() {
    assert(d_currToken.kind == Token::Kind::LiteralInt);
    try {
        std::size_t pos;
        const int64_t value = std::stoll(std::string(d_currToken.text), &pos);
        assert(pos == d_currToken.text.size());
        AstLiteralExpr* res = d_env.createNode<AstLiteralExpr>(d_currToken.location, value);
        getNextToken(); // consume literal
        return res;
    } catch (std::logic_error&) {
        d_env.throwError(d_currToken.location, "Invalid integer literal");
    }
}

} // namespace jex
