#pragma once

#include <jex_location.hpp>

#include <iosfwd>
#include <stdexcept>
#include <string>

namespace jex {

struct MsgInfo {
    enum class Kind {
        Error,
        Warning,
        Info,
        Note
    } kind;
    Location loc;
    std::string msg;

    bool operator<(const MsgInfo& other) const;

    MsgInfo(Kind kind, const Location& loc, std::string msg)
    : kind(kind)
    , loc(loc)
    , msg(std::move(msg)) {
    }
};

std::ostream& operator<<(std::ostream& str, const MsgInfo& info);

class CompileError : public std::runtime_error {
public:
    CompileError(const std::string& error)
    : std::runtime_error(error) {
    }
};

} // namespace jex
