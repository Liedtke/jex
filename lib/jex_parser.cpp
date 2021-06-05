#include <jex_parser.hpp>

#include <jex_ast.hpp>
#include <jex_compileenv.hpp>

#include <sstream>
#include <string>

namespace jex {

Token& Parser::getNextToken() {
    d_currToken = d_lexer.getNext();
    return d_currToken;
}

void Parser::parse() {
    parseExpression();
}

IAstExpression* Parser::parseExpression() {
    switch (d_currToken.kind) {
        case Token::Kind::LiteralInt:
            return parseLiteralInt();
        default:
            std::stringstream msg;
            // TODO: extend expectation list
            msg << "Unexpected " << d_currToken << ", expecting literal";
            d_env.throwError(d_currToken.location, msg.str());
    }
}

AstLiteralExpr* Parser::parseLiteralInt() {
    assert(d_currToken.kind == Token::Kind::LiteralInt);
    try {
        std::size_t pos;
        const int64_t value = std::stoll(std::string(d_currToken.text), &pos);
        return d_env.createNode<AstLiteralExpr>(d_currToken.location, value);
    } catch (std::logic_error&) {
        d_env.throwError(d_currToken.location, "Invalid integer literal");
    }
}

} // namespace jex
