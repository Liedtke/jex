#include <jex_symboltable.hpp>

#include <jex_ast.hpp>
#include <jex_compileenv.hpp>
#include <jex_errorhandling.hpp>
#include <jex_fctinfo.hpp>
#include <jex_fctlibrary.hpp>

namespace jex {

const char* const SymbolTable::s_unresolved = "_unresolved";

SymbolTable::SymbolTable(CompileEnv& env)
: d_env(env) {
    // Add _unresolved type.
    const TypeInfoId typeUnresolved = d_env.typeSystem().unresolved();
    d_symbols[s_unresolved] = std::make_unique<Symbol>(
        Symbol::Kind::Unresolved, s_unresolved, typeUnresolved, nullptr);
    // Add special if function.
    d_symbols["if"] = std::make_unique<Symbol>(
        Symbol::Kind::Function, "if", typeUnresolved, nullptr);
    // add types
    for (auto& typeEntry : d_env.typeSystem()) {
        d_symbols[typeEntry.first] = std::make_unique<Symbol>(Symbol::Kind::Type, typeEntry.first, typeEntry.second, nullptr);
    }
    // add functions
    for (auto& fctEntry : d_env.fctLibrary()) {
        // For constructors this will not overwrite the type and it is OK to have functions named as a Type.
        d_symbols.emplace(fctEntry.first, std::make_unique<Symbol>(Symbol::Kind::Function, fctEntry.first, typeUnresolved, nullptr));
    }
}

bool SymbolTable::resolveSymbol(AstIdentifier* ident) const {
    auto iter = d_symbols.find(ident->d_name);
    if (iter == d_symbols.end()) {
        d_env.createError(ident, "Unknown identifier '" + std::string(ident->d_name) + "'");
        ident->d_symbol = d_symbols.at(s_unresolved).get();
        return false;
    }
    ident->d_symbol = iter->second.get();
    ident->d_resultType = ident->d_symbol->type;
    return true;
}

Symbol* SymbolTable::addSymbol(const Location& loc, Symbol::Kind kind, std::string_view name, TypeInfoId type, AstVariableDef* defNode) {
    auto [iter, inserted] = d_symbols.emplace(name, std::make_unique<Symbol>(kind, name, type, defNode));
    if (!inserted) {
        const MsgInfo& msg = d_env.createError(loc, "Duplicate identifier '" + std::string(name) + "'");
        if (iter->second->defNode != nullptr) {
            msg.addNote(iter->second->defNode->d_loc, "Previously defined here");
        }
        return d_symbols.at(s_unresolved).get();
    }
    return iter->second.get();
}

} // namespace jex
