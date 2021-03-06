#pragma once

#include <jex_base.hpp>
#include <jex_typesystem.hpp>

#include <memory>
#include <string_view>
#include <unordered_map>

namespace jex {

class AstIdentifier;
class AstVariableDef;
class CompileEnv;
struct Location;

struct Symbol {
    enum class Kind {
        Variable,
        Function,
        Type,
        Unresolved
    } kind;
    TypeInfoId type;
    std::string_view name;
    AstVariableDef* defNode;

    Symbol(Kind kind, std::string_view name, TypeInfoId type, AstVariableDef* defNode)
    : kind(kind)
    , type(type)
    , name(name)
    , defNode(defNode) {
    }
};

class SymbolTable : NoCopy {
    CompileEnv& d_env;
    // TODO: Remove unique_ptr, the Symbol can be stored directly in the map.
    std::unordered_map<std::string_view, std::unique_ptr<Symbol>> d_symbols;
    static const char* const s_unresolved;
public:
    SymbolTable(CompileEnv& env);

    bool resolveSymbol(AstIdentifier* ident) const;
    Symbol* addSymbol(const Location& loc, Symbol::Kind kind, std::string_view name, TypeInfoId type, AstVariableDef* defNode = nullptr);
};

} // namespace jex
