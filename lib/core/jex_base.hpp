#pragma once

namespace jex {

/**
 * Base class for defining non-copyable non-movable objects.
 */
class NoCopy {
public:
    NoCopy() = default;
    NoCopy(const NoCopy& other) = delete;
    NoCopy& operator=(NoCopy& other) = delete;
};

/**
 * Overload class for use in combination with std::visit.
 *
 * Example:
 * std::visit(overloaded {
 *     [](int x) { ... },
 *     [](double x) { ...}
 * }, variant);
 */
template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
// Explicit deduction guide (not needed as of C++20).
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

} // namespace jex
