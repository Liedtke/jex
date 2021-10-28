#include <jex_compileenv.hpp>

#include <jex_ast.hpp>
#include <jex_constantstore.hpp>
#include <jex_environment.hpp>
#include <jex_errorhandling.hpp>
#include <jex_fctlibrary.hpp>
#include <jex_symboltable.hpp>
#include <jex_typesystem.hpp>

#include <sstream>

namespace jex {

CompileEnv::CompileEnv(const Environment& env, bool useIntrinsics)
: d_fileName("test") // TODO: Provide real file name
, d_useIntrinsics(useIntrinsics)
, d_messages(std::make_unique<std::set<MsgInfo>>())
, d_typeSystem(env.types())
, d_fctLibrary(env.fctLib())
, d_symbolTable(std::make_unique<SymbolTable>(*this))
, d_constants(std::make_unique<ConstantStore>()) {
};

CompileEnv::~CompileEnv() = default;

const MsgInfo& CompileEnv::createError(const Location& loc, std::string msg) {
    auto inserted = d_messages->emplace(MsgInfo::Kind::Error, loc, std::move(msg));
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

std::unique_ptr<std::set<MsgInfo>> CompileEnv::releaseMessages() {
    return std::move(d_messages);
}

std::unique_ptr<ConstantStore> CompileEnv::releaseConstants() {
    return std::move(d_constants);
}

} // namespace jex
