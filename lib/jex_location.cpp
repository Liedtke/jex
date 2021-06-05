#include <jex_location.hpp>

#include <iostream>

namespace jex {

std::ostream& operator<<(std::ostream& str, const Location& loc) {
    return str << loc.lineBegin << '.' << loc.colBegin << '-' << loc.lineEnd << '.' << loc.colEnd;
}

// TODO: use spaceship operator instead
bool Location::operator==(const Location& other) const {
    return lineBegin == other.lineBegin
        && colBegin == other.colBegin
        && lineEnd == other.lineEnd
        && colEnd == other.colEnd;
}

bool Location::operator<(const Location& other) const {
    return lineBegin < other.lineBegin
        || (lineBegin == other.lineBegin
        && (colBegin < other.colBegin
        || (colBegin == other.colBegin
        && (lineEnd < other.lineEnd
        || (lineEnd == other.lineEnd
        && colEnd < other.colEnd)))));
}

} // namespace jex
