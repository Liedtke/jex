#include <jex_compileenv.hpp>

#include <jex_ast.hpp>
#include <jex_errorhandling.hpp>

#include <sstream>

namespace jex {

CompileEnv::~CompileEnv() {
}

const MsgInfo& CompileEnv::createError(const Location& loc, std::string msg) {
    auto inserted = d_messages.emplace(MsgInfo::Kind::Error, loc, std::move(msg));
    assert(inserted.second);
    return *inserted.first;
}

[[noreturn]] void CompileEnv::throwError(const Location& loc, std::string msg) {
    const MsgInfo& msgInfo = createError(loc, std::move(msg));
    std::stringstream error;
    error << msgInfo;
    throw CompileError(error.str());
}

} // namespace jex
