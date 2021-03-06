#include <jex_prettyprinter.hpp>

#include <jex_ast.hpp>
#include <jex_errorhandling.hpp>

#include <iostream>

namespace jex {

void PrettyPrinter::visit(AstLiteralExpr& node) {
    std::visit(overloaded {
        [&](bool val) { d_str << (val ? "true" : "false"); },
        [&](std::string_view val) { d_str << '"' << val << '"'; },
        [&](auto&& val) { d_str << val; },
    }, node.d_value);
}

void PrettyPrinter::visit(AstBinaryExpr& node) {
    d_str << "(";
    node.d_lhs->accept(*this);
    switch(node.d_op) {
        case OpType::Add:
            d_str << " + ";
            break;
        case OpType::Sub:
            d_str << " - ";
            break;
        case OpType::Mul:
            d_str << " * ";
            break;
        case OpType::Div:
            d_str << " / ";
            break;
        case OpType::Mod:
            d_str << " % ";
            break;
        case OpType::EQ:
            d_str << " == ";
            break;
        case OpType::NE:
            d_str << " != ";
            break;
        case OpType::LT:
            d_str << " < ";
            break;
        case OpType::GT:
            d_str << " > ";
            break;
        case OpType::LE:
            d_str << " <= ";
            break;
        case OpType::GE:
            d_str << " >= ";
            break;
        case OpType::BitAnd:
            d_str << " & ";
            break;
        case OpType::BitOr:
            d_str << " | ";
            break;
        case OpType::BitXor:
            d_str << " ^ ";
            break;
        case OpType::And:
            d_str << " && ";
            break;
        case OpType::Or:
            d_str << " || ";
            break;
        case OpType::Shl:
            d_str << " shl ";
            break;
        case OpType::Shrs:
            d_str << " shrs ";
            break;
        case OpType::Shrz:
            d_str << " shrz ";
            break;
        default: // LCOV_EXCL_LINE
            assert(false && "Invalid op type for binary expression"); // LCOV_EXCL_LINE
    }
    node.d_rhs->accept(*this);
    d_str << ")";
}

void PrettyPrinter::visit(AstLogicalBinExpr& node) {
    visit(static_cast<AstBinaryExpr&>(node));
}

void PrettyPrinter::visit(AstUnaryExpr& node) {
    if (node.d_op == OpType::UMinus) {
        d_str << '-';
    } else {
        assert(node.d_op == OpType::Not && "Unary expression has to be '-' or '!'");
        d_str << '!';
    }
    node.d_expr->accept(*this);
}

void PrettyPrinter::visit(AstFctCall& node) {
    node.d_fct->accept(*this);
    d_str << "(";
    node.d_args->accept(*this);
    d_str << ")";
}

void PrettyPrinter::visit(AstIf& node) {
    PrettyPrinter::visit(static_cast<AstFctCall&>(node));
}

void PrettyPrinter::visit(AstIdentifier& node) {
    d_str << node.d_name;
}

void PrettyPrinter::visit(AstVarArg& node) {
    d_str << "[";
    bool first = true;
    for (IAstExpression* expr : node.d_args) {
        if (first) {
            first = false;
        } else {
            d_str << ", ";
        }
        expr->accept(*this);
    }
    d_str << "]";
}

void PrettyPrinter::visit(AstArgList& node) {
    bool first = true;
    for (IAstExpression* expr : node.d_args) {
        if (first) {
            first = false;
        } else {
            d_str << ", ";
        }
        expr->accept(*this);
    }
}

void PrettyPrinter::visit(AstVariableDef& node) {
    switch(node.d_kind) {
        case VariableKind::Const:
            d_str << "const ";
            break;
        case VariableKind::Var:
            d_str << "var ";
            break;
        case VariableKind::Expr:
            d_str << "expr ";
            break;
    }
    node.d_name->accept(*this);
    d_str << ": ";
    node.d_type->accept(*this);
    if (node.d_expr != nullptr) {
        d_str << " = ";
        node.d_expr->accept(*this);
    }
    d_str << ";\n";
}

void PrettyPrinter::visit(AstConstantExpr& node) {
    d_str << '[' << node.d_constantName << ']';
}

void PrettyPrinter::visit(AstRoot& node) {
    for (AstVariableDef* def : node.d_varDefs) {
        def->accept(*this);
    }
}

} // namespace jex
