#include <jex_parser.hpp>

#include <jex_ast.hpp>
#include <jex_compileenv.hpp>
#include <jex_symboltable.hpp>

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
        default: // LCOV_EXCL_LINE
            env.throwError(op.location, "Invalid operator '" + std::string(op.text) + "'"); // LCOV_EXCL_LINE
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
    if (d_currToken.kind != Token::Kind::Eof) {
        std::stringstream msg;
        msg << "Unexpected " << d_currToken << ", expecting an operator or end of file";
        d_env.throwError(d_currToken.location, msg.str());
    }
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

AstArgList* Parser::parseArgList() {
    assert(d_currToken.kind == Token::Kind::ParensL);
    getNextToken(); // consume '('
    AstArgList* argList = d_env.createNode<AstArgList>(d_currToken.location);
    if (d_currToken.kind == Token::Kind::ParensR) {
        // empty argument list
        getNextToken(); // consume ')'
        return argList;
    }
    while (true) {
        IAstExpression* arg = parseExpression();
        argList->addArg(arg);
        switch (d_currToken.kind) {
            case Token::Kind::Comma:
                getNextToken(); // consume ','
                break;
            case Token::Kind::ParensR:
                getNextToken(); // consume ')'
                return argList;
            default: {
                std::stringstream msg;
                msg << "Unexpected " << d_currToken << ", expecting ',' or ')'";
                d_env.throwError(d_currToken.location, msg.str());
            }
        }
    }
}

IAstExpression* Parser::parseIdentOrCall() {
    assert(d_currToken.kind == Token::Kind::Ident);
    AstIdentifier *ident = d_env.createNode<AstIdentifier>(d_currToken.location, d_currToken.text);
    d_env.symbols().resolveSymbol(ident);
    assert(ident->d_symbol != nullptr);
    getNextToken(); // consume identifier

    // parse function call
    if (d_currToken.kind == Token::Kind::ParensL) {
        AstArgList* args = parseArgList();
        return d_env.createNode<AstFctCall>(Location::combine(ident->d_loc, args->d_loc), ident, args);
    }
    // regular identifier
    return ident;
}

IAstExpression* Parser::parsePrimary() {
    switch (d_currToken.kind) {
        case Token::Kind::LiteralInt:
            return parseLiteralInt();
        case Token::Kind::LiteralFloat:
            return parseLiteralFloat();
        case Token::Kind::LiteralString:
            return parseLiteralString();
        case Token::Kind::ParensL:
            return parseParensExpr();
        case Token::Kind::Ident:
            return parseIdentOrCall();
        default:
            std::stringstream msg;
            // TODO: extend expectation list
            msg << "Unexpected " << d_currToken << ", expecting literal, identifier or '('";
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

AstLiteralExpr* Parser::parseLiteralFloat() {
    assert(d_currToken.kind == Token::Kind::LiteralFloat);
    try {
        std::size_t pos;
        const double value = std::stod(std::string(d_currToken.text), &pos);
        if (pos != d_currToken.text.size()) {
            d_env.throwError(d_currToken.location, "Invalid floating point literal");
        }
        AstLiteralExpr* res = d_env.createNode<AstLiteralExpr>(d_currToken.location, value);
        getNextToken(); // consume literal
        return res;
    } catch (std::logic_error&) {
        d_env.throwError(d_currToken.location, "Invalid floating point literal");
    }
}

AstLiteralExpr* Parser::parseLiteralString() {
    AstLiteralExpr* res = d_env.createNode<AstLiteralExpr>(d_currToken.location, d_currToken.text);
    getNextToken(); // consume literal
    return res;
}

} // namespace jex
