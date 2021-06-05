#pragma once

#include <iosfwd>

namespace jex {

struct Location {
    int lineBegin = 1;
    int colBegin = 1;
    int lineEnd = 1;
    int colEnd = 1;

    bool operator==(const Location& other) const;
    bool operator<(const Location& other) const;
};

std::ostream& operator<<(std::ostream& str, const Location& loc);

} // namespace jex
