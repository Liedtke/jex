#include <jex_errorhandling.hpp>

#include <iostream>
#include <sstream>

#include <cassert>

namespace jex {

bool MsgInfo::operator<(const MsgInfo& other) const {
    return loc < other.loc || (loc == other.loc && msg < other.msg);
}

std::ostream& operator<<(std::ostream& str, const MsgInfo::Kind& info) {
    switch (info) {
        case MsgInfo::Kind::Note:
            return str << "Note";
        case MsgInfo::Kind::Error:
            return str << "Error";
    }
    throw InternalError("<unknown message kind>"); // LCOV_EXCL_LINE
}

std::ostream& operator<<(std::ostream& str, const MsgInfo& info) {
    // TODO: add error kind to output
    str << info.loc << ": " << info.kind << ": " << info.msg;
    if (info.d_note) {
        str << '\n' << *info.d_note;
    }
    return str;
}

CompileError CompileError::create(const MsgInfo& msgInfo) {
    std::stringstream str;
    str << msgInfo;
    return CompileError(str.str());
}

const MsgInfo& MsgInfo::addNote(const Location& loc, std::string msg) const {
    d_note = std::make_unique<MsgInfo>(Kind::Note, loc, std::move(msg));
    return *d_note;
}

} // namespace jex
