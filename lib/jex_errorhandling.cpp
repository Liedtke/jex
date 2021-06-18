#include <jex_errorhandling.hpp>

#include <iostream>
#include <sstream>

namespace jex {

bool MsgInfo::operator<(const MsgInfo& other) const {
    return loc < other.loc || (loc == other.loc && msg < other.msg);
}

std::ostream& operator<<(std::ostream& str, const MsgInfo& info) {
    // TODO: add error kind to output
    str << info.loc << ": " << info.msg;
    if (info.d_note) {
        str << '\n' << *info.d_note;
    }
    return str;
}

CompileError CompileError::create(const Location& loc, std::string msg) {
    std::stringstream str;
    str << MsgInfo(MsgInfo::Kind::Error, loc, std::move(msg));
    return CompileError(str.str());
}

const MsgInfo& MsgInfo::addNote(const Location& loc, std::string msg) const {
    d_note = std::make_unique<MsgInfo>(Kind::Note, loc, std::move(msg));
    return *d_note;
}

} // namespace jex
