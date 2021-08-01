#include <jex_compileenv.hpp>

#include <jex_ast.hpp>
#include <jex_errorhandling.hpp>
#include <jex_fctlibrary.hpp>
#include <jex_symboltable.hpp>
#include <jex_typesystem.hpp>

#include <sstream>

namespace jex {

CompileEnv::CompileEnv()
: d_typeSystem(std::make_unique<TypeSystem>())
, d_fctLibrary(std::make_unique<FctLibrary>())
, d_symbolTable(std::make_unique<SymbolTable>(*this)) {
};

CompileEnv::~CompileEnv() {
}

const MsgInfo& CompileEnv::createError(const Location& loc, std::string msg) {
    auto inserted = d_messages.emplace(MsgInfo::Kind::Error, loc, std::move(msg));
    assert(inserted.second);
    d_hasErrors = true;
    return *inserted.first;
}

[[noreturn]] void CompileEnv::throwError(const Location& loc, std::string msg) {
    const MsgInfo& msgInfo = createError(loc, std::move(msg));
    std::stringstream error;
    error << msgInfo;
    throw CompileError(error.str());
}

std::string_view CompileEnv::createStringLiteral(std::string_view str) {
    return d_stringLiterals.emplace_back(str);
}

} // namespace jex
