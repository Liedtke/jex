#pragma once

#include <jex_typesystem.hpp>

#include <memory>
#include <string_view>
#include <unordered_map>

namespace jex {

class AstIdentifier;
class CompileEnv;
class IAstNode;
struct Location;

struct Symbol {
    enum class Kind {
        Variable,
        Function,
        Unresolved
    } kind;
    TypeId type;
    std::string_view name;
    IAstNode* defNode;

    Symbol(Kind kind, std::string_view name, IAstNode* defNode, TypeId type = TypeId::Unresolved)
    : kind(kind)
    , type(type)
    , name(name)
    , defNode(defNode) {
    }
};

class SymbolTable {
    CompileEnv& d_env;
    // TODO: Remove unique_ptr, the Symbol can be stored directly in the map.
    std::unordered_map<std::string_view, std::unique_ptr<Symbol>> d_symbols;
    static const char* const s_unresolved;
public:
    SymbolTable(CompileEnv& env);

    bool resolveSymbol(AstIdentifier* ident) const;
    Symbol* addSymbol(const Location& loc, Symbol::Kind kind, std::string_view name, IAstNode* defNode = nullptr);
};

} // namespace jex
