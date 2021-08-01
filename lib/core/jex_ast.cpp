#include <jex_ast.hpp>

namespace jex {

void AstArgList::addArg(IAstExpression* arg) {
    d_args.push_back(arg);
    d_loc = Location::combine(d_loc, arg->d_loc);
}

} // namespace jex
