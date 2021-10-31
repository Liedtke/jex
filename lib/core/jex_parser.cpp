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
        default: // LCOV_EXCL_LINE
            env.throwError(op.location, "Invalid operator '" + std::string(op.text) + "'"); // LCOV_EXCL_LINE
    }
}

} // anonymous namespace

void Parser::initPrecs() {
    // == !=
    d_precs[Token::Kind::OpEQ] = 10;
    d_precs[Token::Kind::OpNE] = 10;
    // < <= > >=
    d_precs[Token::Kind::OpLT] = 20;
    d_precs[Token::Kind::OpLE] = 20;
    d_precs[Token::Kind::OpGT] = 20;
    d_precs[Token::Kind::OpGE] = 20;
    // + -
    d_precs[Token::Kind::OpAdd] = 30;
    d_precs[Token::Kind::OpSub] = 30;
    // * / %
    d_precs[Token::Kind::OpMul] = 40;
    d_precs[Token::Kind::OpDiv] = 40;
    d_precs[Token::Kind::OpMod] = 40;
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
                root->d_varDefs.push_back(parseVariableDef());
                break;
            default:
                throwUnexpected("'var' or end of file");
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
        if (symKind != Symbol::Kind::Function && symKind != Symbol::Kind::Unresolved) {
            d_env.createError(ident->d_loc, "Invalid call: '" + std::string(ident->d_name) + "' is not a function");
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
        d_env.createError(ident->d_loc, "Invalid expression: '" + std::string(ident->d_name) + "' is not a variable");
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
        lhs = d_env.createNode<AstBinaryExpr>(loc, unresolved, getOp(binOp, d_env), lhs, rhs);
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

AstVariableDef* Parser::parseVariableDef() {
    assert(d_currToken.kind == Token::Kind::Var);
    const Location beginLoc = d_currToken.location;
    getNextToken(); // consume 'var'
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
        d_env.createError(type->d_loc, "Invalid type: '" + std::string(type->d_name) + "' is not a type");
    }
    name->d_symbol = d_env.symbols().addSymbol(name->d_loc, Symbol::Kind::Variable, name->d_name, type->d_resultType);
    if (d_currToken.kind != Token::Kind::Assign) {
        throwUnexpected("'='");
    }
    getNextToken();
    IAstExpression* expr = parseExpression();
    if (d_currToken.kind != Token::Kind::Semicolon) {
        throwUnexpected("';'");
    }
    getNextToken();
    const Location loc = Location::combine(beginLoc, expr->d_loc);
    AstVariableDef* varDef = d_env.createNode<AstVariableDef>(loc, name, type, expr);
    name->d_symbol->defNode = varDef;
    return varDef;
}

[[noreturn]] void Parser::throwUnexpected(std::string_view expecting) {
    std::stringstream msg;
    msg << "Unexpected " << d_currToken << ", expecting " << expecting;
    d_env.throwError(d_currToken.location, msg.str());
}

} // namespace jex
