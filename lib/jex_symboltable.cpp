#include <jex_symboltable.hpp>

#include <jex_ast.hpp>
#include <jex_compileenv.hpp>
#include <jex_errorhandling.hpp>

namespace jex {

const char* const SymbolTable::s_unresolved = "_unresolved";

SymbolTable::SymbolTable(CompileEnv& env)
: d_env(env) {
    d_symbols[s_unresolved] = std::make_unique<Symbol>(Symbol::Kind::Unresolved, s_unresolved, Type::Unresolved);
}

bool SymbolTable::resolveSymbol(AstIdentifier* ident) const {
    auto iter = d_symbols.find(ident->d_name);
    if (iter == d_symbols.end()) {
        d_env.createError(ident->d_loc, "Unknown identifier '" + std::string(ident->d_name) + "'");
        ident->d_symbol = d_symbols.at(s_unresolved).get();
        return false;
    }
    ident->d_symbol = iter->second.get();
    return true;
}

Symbol* SymbolTable::addSymbol(const Location& loc, Symbol::Kind kind, std::string_view name) {
    auto [iter, inserted] = d_symbols.emplace(name, std::make_unique<Symbol>(kind, name));
    if (!inserted) {
        const MsgInfo& msg = d_env.createError(loc, "Duplicate identifier '" + std::string(name) + "'");
        msg.addNote(loc, "Previously defined here"); // TODO: store location in symbol and use it here
        return d_symbols.at(s_unresolved).get();
    }
    return iter->second.get();
}

} // namespace jex
