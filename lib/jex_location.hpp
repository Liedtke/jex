#pragma once

#include <iosfwd>

namespace jex {

struct CodePos {
    int line = 1;
    int col = 1;

    bool operator==(const CodePos& other) const;
    bool operator<(const CodePos& other) const;
};

struct Location {
    CodePos begin;
    CodePos end;

    bool operator==(const Location& other) const;
    bool operator<(const Location& other) const;

    static Location combine(const Location& a, const Location& b);
};

std::ostream& operator<<(std::ostream& str, const Location& loc);

} // namespace jex
