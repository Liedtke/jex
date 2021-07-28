#include <jex_typeinference.hpp>

#include <jex_ast.hpp>
#include <jex_compileenv.hpp>
#include <jex_symboltable.hpp>
#include <jex_typesystem.hpp>

#include <cassert>

namespace jex {

void TypeInference::visit(AstLiteralExpr& node) {
    BasicAstVisitor::visit(node);
    // Literals are already resolved by the Parser.
    assert(d_env.typeSystem().isResolved(node.d_resultType));
}

void TypeInference::visit(AstFctCall& node) {
    // TODO: Resolve function calls. This requires some kind of function library!
}

void TypeInference::visit(AstIdentifier& node) {
    // Identifiers are already resolved.
    assert(d_env.typeSystem().isResolved(node.d_resultType));
}

} // namespace jex
