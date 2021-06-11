#include <jex_location.hpp>

#include <algorithm>
#include <iostream>

namespace jex {

bool CodePos::operator==(const CodePos& other) const {
    return line == other.line && col == other.col;
}

bool CodePos::operator<(const CodePos& other) const {
    return line < other.line || (line == other.line && col < other.col);
}

// TODO: use spaceship operator instead
bool Location::operator==(const Location& other) const {
    return begin == other.begin && end == other.end;
}

bool Location::operator<(const Location& other) const {
    return begin < other.begin
        || (begin == other.begin && end < other.end);
}

Location Location::combine(const Location& a, const Location& b) {
    return {std::min(a.begin, b.begin), std::max(a.end, b.end)};
}

std::ostream& operator<<(std::ostream& str, const Location& loc) {
    return str << loc.begin.line << '.' << loc.begin.col << '-' << loc.end.line << '.' << loc.end.col;
}

} // namespace jex
