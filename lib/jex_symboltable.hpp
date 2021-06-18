#pragma once

#include <jex_typesystem.hpp>

#include <memory>
#include <string_view>
#include <unordered_map>

namespace jex {

class AstIdentifier;
class CompileEnv;
struct Location;

struct Symbol {
    enum class Kind {
        Variable,
        Function,
        Unresolved
    } kind;
    Type type;
    std::string_view name;

    Symbol(Kind kind, std::string_view name, Type type = Type::Unresolved)
    : kind(kind)
    , type(type)
    , name(name) {
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
    Symbol* addSymbol(const Location& loc, Symbol::Kind kind, std::string_view name);
};

} // namespace jex
