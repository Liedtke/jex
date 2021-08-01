#pragma once

#include <jex_location.hpp>

#include <iosfwd>
#include <memory>
#include <stdexcept>
#include <string>

namespace jex {

/**
 * Defines a message created during compilation.
 * Currently these may only be errors.
 * MsgInfos my furthermore have additional notes attached which itself
 * are MsgInfos as well.
 */
struct MsgInfo {
    enum class Kind {
        Error,
        Note
    } kind;
    Location loc;
    std::string msg;
    mutable std::unique_ptr<MsgInfo> d_note;

    bool operator<(const MsgInfo& other) const;

    MsgInfo(Kind kind, const Location& loc, std::string msg)
    : kind(kind)
    , loc(loc)
    , msg(std::move(msg)) {
    }

    const MsgInfo& addNote(const Location& loc, std::string msg) const;
};

std::ostream& operator<<(std::ostream& str, const MsgInfo::Kind& info);
std::ostream& operator<<(std::ostream& str, const MsgInfo& info);

/**
 * Defines a hard error during compilation.
 */
class CompileError : public std::runtime_error {
public:
    CompileError(const std::string& error)
    : std::runtime_error(error) {
    }

    static CompileError create(const Location& loc, std::string msg);
    static CompileError create(const MsgInfo& msgInfo);
};

/**
 * Defines an internal error.
 * Internal errors are caused by a misuse of the compilation interface or type / function
 * registration mechanism. They might also be thrown as a result of a bug in the compiler but are
 * not expected as a result of invalid source code.
 */
class InternalError : public std::runtime_error {
public:
    InternalError(const std::string& error)
    : std::runtime_error(error) {
    }
};

} // namespace jex
