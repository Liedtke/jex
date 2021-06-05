#include <jex_errorhandling.hpp>

#include <iostream>

namespace jex {

bool MsgInfo::operator<(const MsgInfo& other) const {
    return loc < other.loc || (loc == other.loc && msg < other.msg);
}

std::ostream& operator<<(std::ostream& str, const MsgInfo& info) {
    return str << info.loc << ": " << info.msg;
}

} // namespace jex
