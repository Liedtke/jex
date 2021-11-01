#include <jex_ast.hpp>

#include <sstream>

namespace jex {

void AstArgList::addArg(IAstExpression* arg) {
    d_args.push_back(arg);
    d_loc = Location::combine(d_loc, arg->d_loc);
}

void AstVarArg::addArg(IAstExpression* arg) {
    d_args.push_back(arg);
    d_loc = Location::combine(d_loc, arg->d_loc);
}

AstConstantExpr::AstConstantExpr(IAstExpression& replaced)
: IAstExpression(replaced.d_loc, replaced.d_resultType) {
    std::stringstream name;
    name << "const_" << d_resultType->name() << "_l" << d_loc.begin.line << "_c" << d_loc.begin.col;
    d_constantName = name.str();
}

} // namespace jex
