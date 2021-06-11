#include <jex_errorhandling.hpp>

#include <iostream>
#include <sstream>

namespace jex {

bool MsgInfo::operator<(const MsgInfo& other) const {
    return loc < other.loc || (loc == other.loc && msg < other.msg);
}

std::ostream& operator<<(std::ostream& str, const MsgInfo& info) {
    return str << info.loc << ": " << info.msg;
}

CompileError CompileError::create(const Location& loc, std::string msg) {
    std::stringstream str;
    str << MsgInfo(MsgInfo::Kind::Error, loc, std::move(msg));
    return CompileError(str.str());
}

} // namespace jex
