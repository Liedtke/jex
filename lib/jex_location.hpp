#pragma once

#include <iosfwd>

namespace jex {

/**
 * Represents a position in the source code.
 */
struct CodePos {
    int line = 1;
    int col = 1;

    bool operator==(const CodePos& other) const;
    bool operator<(const CodePos& other) const;
};

/**
 * Represents a location in the source code containing a begin and an end position.
 */
struct Location {
    CodePos begin;
    CodePos end;

    bool operator==(const Location& other) const;
    bool operator<(const Location& other) const;

    /**
     * Combine two Locations together.
     * The new location will span from the smallest begin position to the largest end position.
     * The new location will also span over any gap between the two locations if they are not
     * adjacent.
     */
    static Location combine(const Location& a, const Location& b);
};

std::ostream& operator<<(std::ostream& str, const Location& loc);

} // namespace jex
