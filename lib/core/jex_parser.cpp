#include <jex_parser.hpp>

#include <jex_ast.hpp>
#include <jex_compileenv.hpp>
#include <jex_errorhandling.hpp>
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
        case Token::Kind::OpEQ:
            return OpType::EQ;
        case Token::Kind::OpNE:
            return OpType::NE;
        case Token::Kind::OpLT:
            return OpType::LT;
        case Token::Kind::OpGT:
            return OpType::GT;
        case Token::Kind::OpLE:
            return OpType::LE;
        case Token::Kind::OpGE:
            return OpType::GE;
        case Token::Kind::OpBitAnd:
            return OpType::BitAnd;
        case Token::Kind::OpBitOr:
            return OpType::BitOr;
        case Token::Kind::OpBitXor:
            return OpType::BitXor;
        case Token::Kind::OpAnd:
            return OpType::And;
        case Token::Kind::OpOr:
            return OpType::Or;
        case Token::Kind::OpShl:
            return OpType::Shl;
        case Token::Kind::OpShrs:
            return OpType::Shrs;
        case Token::Kind::OpShrz:
            return OpType::Shrz;
        default: // LCOV_EXCL_LINE
            env.throwError(op.location, "Invalid operator '" + std::string(op.text) + "'"); // LCOV_EXCL_LINE
    }
}

} // anonymous namespace

void Parser::initPrecs() {
    // logical
    d_precs[Token::Kind::OpOr] = 1;
    d_precs[Token::Kind::OpAnd] = 2;
    // bitwise
    d_precs[Token::Kind::OpBitOr] = 10;
    d_precs[Token::Kind::OpBitXor] = 11;
    d_precs[Token::Kind::OpBitAnd] = 12;
    // == !=
    d_precs[Token::Kind::OpEQ] = 20;
    d_precs[Token::Kind::OpNE] = 20;
    // < <= > >=
    d_precs[Token::Kind::OpLT] = 30;
    d_precs[Token::Kind::OpLE] = 30;
    d_precs[Token::Kind::OpGT] = 30;
    d_precs[Token::Kind::OpGE] = 30;
    // shl, shrs, shrz
    d_precs[Token::Kind::OpShl] = 40;
    d_precs[Token::Kind::OpShrs] = 40;
    d_precs[Token::Kind::OpShrz] = 40;
    // + -
    d_precs[Token::Kind::OpAdd] = 50;
    d_precs[Token::Kind::OpSub] = 50;
    // * / %
    d_precs[Token::Kind::OpMul] = 60;
    d_precs[Token::Kind::OpDiv] = 60;
    d_precs[Token::Kind::OpMod] = 60;
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
    d_env.setRoot(parseRoot());
    assert(d_currToken.kind == Token::Kind::Eof);
    if (d_env.hasErrors()) {
        // Throw first error in list.
        throw CompileError::create(*d_env.messages().begin());
    }
}

AstRoot* Parser::parseRoot() {
    AstRoot* root = d_env.createNode<AstRoot>(d_currToken.location);
    while (true) {
        switch (d_currToken.kind) {
            case Token::Kind::Eof:
                return root;
            case Token::Kind::Var:
            case Token::Kind::Const:
            case Token::Kind::Expr:
                root->d_varDefs.push_back(parseVariableDef());
                break;
            default:
                throwUnexpected("'var', 'const', 'expr' or end of file");
        }
    }
}

IAstExpression* Parser::parseParensExpr() {
    assert(d_currToken.kind == Token::Kind::ParensL);
    getNextToken();
    IAstExpression* expr = parseExpression();
    if (d_currToken.kind != Token::Kind::ParensR) {
        throwUnexpected("')'");
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
                argList->d_loc = Location::combine(argList->d_loc, d_currToken.location);
                getNextToken(); // consume ')'
                return argList;
            default:
                throwUnexpected("',' or ')'");
        }
    }
}

AstIdentifier* Parser::parseIdent() {
    assert(d_currToken.kind == Token::Kind::Ident);
    TypeInfoId unresolved = d_env.typeSystem().unresolved();
    AstIdentifier *ident = d_env.createNode<AstIdentifier>(d_currToken.location, unresolved, d_currToken.text);
    getNextToken(); // consume identifier
    return ident;
}

IAstExpression* Parser::parseIdentOrCall() {
    AstIdentifier* ident = parseIdent();
    d_env.symbols().resolveSymbol(ident);
    assert(ident->d_symbol != nullptr);
    // parse function call
    if (d_currToken.kind == Token::Kind::ParensL) {
        Symbol::Kind symKind = ident->d_symbol->kind;
        if (symKind != Symbol::Kind::Function && symKind != Symbol::Kind::Type && symKind != Symbol::Kind::Unresolved) {
            d_env.createError(ident, "Invalid call: '" + std::string(ident->d_name) + "' is not a function");
        }
        AstArgList* args = parseArgList();
        TypeInfoId unresolved = d_env.typeSystem().unresolved();
        if (ident->d_name == "if") {
            // Special node for if which behaves differently to regular function calls.
            return d_env.createNode<AstIf>(Location::combine(ident->d_loc, args->d_loc), unresolved, ident, args);
        }
        return d_env.createNode<AstFctCall>(Location::combine(ident->d_loc, args->d_loc), unresolved, ident, args);
    }
    // regular identifier
    Symbol::Kind symKind = ident->d_symbol->kind;
    if (symKind != Symbol::Kind::Variable && symKind != Symbol::Kind::Unresolved) {
        d_env.createError(ident, "Invalid expression: '" + std::string(ident->d_name) + "' is not a variable");
    }
    return ident;
}

IAstExpression* Parser::parsePrimary() {
    switch (d_currToken.kind) {
        case Token::Kind::LiteralBool:
            return parseLiteralBool();
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
        case Token::Kind::OpSub:
            return parseUnaryMinus();
        case Token::Kind::OpNot:
            return parseUnaryNot();
        default:
            // TODO: extend expectation list
            throwUnexpected("literal, identifier, '-' or '('");
    }
}

IAstExpression* Parser::parseUnaryMinus() {
    Token minus = d_currToken;
    getNextToken(); // consume '-'
    if (d_currToken.kind == Token::Kind::LiteralInt) {
        // Special handling for literal int to support int64::min().
        return parseLiteralInt(true);
    }
    // Unary minus has highest precedence currently.
    IAstExpression* inner = parsePrimary();;
    Location loc = Location::combine(inner->d_loc, minus.location);
    TypeInfoId unresolved = d_env.typeSystem().unresolved();
    return d_env.createNode<AstUnaryExpr>(loc, unresolved, OpType::UMinus, inner);
}

IAstExpression* Parser::parseUnaryNot() {
    Token minus = d_currToken;
    getNextToken(); // consume '!'
    IAstExpression* inner = parsePrimary();;
    Location loc = Location::combine(inner->d_loc, minus.location);
    TypeInfoId unresolved = d_env.typeSystem().unresolved();
    return d_env.createNode<AstUnaryExpr>(loc, unresolved, OpType::Not, inner);
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
        TypeInfoId unresolved = d_env.typeSystem().unresolved();
        OpType op = getOp(binOp, d_env);
        if (op == OpType::And || op == OpType::Or) {
            lhs = d_env.createNode<AstLogicalBinExpr>(loc, unresolved, op, lhs, rhs);
        } else {
            lhs = d_env.createNode<AstBinaryExpr>(loc, unresolved, op, lhs, rhs);
        }
    }
}

IAstExpression* Parser::parseExpression() {
    IAstExpression* lhs = parsePrimary();
    return parseBinOpRhs(0, lhs);
}

AstLiteralExpr* Parser::parseLiteralInt(bool isNegative) {
    assert(d_currToken.kind == Token::Kind::LiteralInt);
    std::string str(isNegative ? "-" : "");
    str += d_currToken.text;
    try {
        std::size_t pos;
        const int64_t value = std::stoll(str, &pos);
        assert(pos == str.size());
        TypeInfoId type = d_env.typeSystem().getType("Integer");
        AstLiteralExpr* res = d_env.createNode<AstLiteralExpr>(d_currToken.location, type, value);
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
        TypeInfoId type = d_env.typeSystem().getType("Float");
        AstLiteralExpr* res = d_env.createNode<AstLiteralExpr>(d_currToken.location, type, value);
        getNextToken(); // consume literal
        return res;
    } catch (std::logic_error&) {
        d_env.throwError(d_currToken.location, "Invalid floating point literal");
    }
}

AstLiteralExpr* Parser::parseLiteralString() {
    assert(d_currToken.kind == Token::Kind::LiteralString);
    TypeInfoId type = d_env.typeSystem().getType("String");
    AstLiteralExpr* res = d_env.createNode<AstLiteralExpr>(d_currToken.location, type, d_currToken.text);
    getNextToken(); // consume literal
    return res;
}

AstLiteralExpr* Parser::parseLiteralBool() {
    assert(d_currToken.kind == Token::Kind::LiteralBool);
    const bool value = d_currToken.text == "true";
    TypeInfoId type = d_env.typeSystem().getType("Bool");
    AstLiteralExpr* res = d_env.createNode<AstLiteralExpr>(d_currToken.location, type, value);
    getNextToken(); // consume literal
    return res;
}

static VariableKind getVariableKind(const Token& token) {
    if (token.kind == Token::Kind::Var) {
        return VariableKind::Var;
    } else if (token.kind == Token::Kind::Expr) {
        return VariableKind::Expr;
    } else {
        assert(token.kind == Token::Kind::Const);
        return VariableKind::Const;
    }
}

AstVariableDef* Parser::parseVariableDef() {
    assert(d_currToken.kind == Token::Kind::Var || d_currToken.kind == Token::Kind::Const || d_currToken.kind == Token::Kind::Expr);
    Location loc = d_currToken.location;
    VariableKind varKind = getVariableKind(d_currToken);
    getNextToken(); // consume variable kind keyword.
    if (d_currToken.kind != Token::Kind::Ident) {
        throwUnexpected("identifier");
    }
    AstIdentifier* name = parseIdent();
    if (d_currToken.kind != Token::Kind::Colon) {
        throwUnexpected("':'");
    }
    getNextToken();
    if (d_currToken.kind != Token::Kind::Ident) {
        throwUnexpected("identifier");
    }
    AstIdentifier* type = parseIdent();
    // Resolve type and register variable in symbol table.
    d_env.symbols().resolveSymbol(type);
    if (type->d_symbol->kind != Symbol::Kind::Type && type->d_symbol->kind != Symbol::Kind::Unresolved) {
        d_env.createError(type, "Invalid type: '" + std::string(type->d_name) + "' is not a type");
    }
    name->d_symbol = d_env.symbols().addSymbol(name->d_loc, Symbol::Kind::Variable, name->d_name, type->d_resultType);
    IAstExpression* expr = nullptr;
    if (varKind != VariableKind::Var) {
        if (d_currToken.kind != Token::Kind::Assign) {
            throwUnexpected("'='");
        }
        getNextToken();
        expr = parseExpression();
        loc = Location::combine(loc, expr->d_loc);
    }
    expect(Token::Kind::Semicolon, "';'");
    getNextToken();
    AstVariableDef* varDef = d_env.createNode<AstVariableDef>(loc, name, type, expr, varKind);
    name->d_symbol->defNode = varDef;
    return varDef;
}

[[noreturn]] void Parser::throwUnexpected(std::string_view expecting) {
    std::stringstream msg;
    msg << "Unexpected " << d_currToken << ", expecting " << expecting;
    d_env.throwError(d_currToken.location, msg.str());
}

void Parser::expect(Token::Kind token, std::string_view expectingMsg) {
    if (d_currToken.kind != token) {
        throwUnexpected(expectingMsg);
    }
}

} // namespace jex
