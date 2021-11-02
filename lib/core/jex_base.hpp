#pragma once

#include <cassert>
#include <type_traits>

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

/**
 * Static cast a non-null pointer to a polymorphic object.
 */
template<typename TargetT, typename SourceT>
inline TargetT cast_ensured(SourceT* ptr) {
    static_assert(std::is_pointer_v<TargetT>, "Target type of cast has to be a pointer type");
    assert(ptr != nullptr && "Pointer may not be null");
    assert(dynamic_cast<TargetT>(ptr) != nullptr && "Cast failed");
    return static_cast<TargetT>(ptr);
}

} // namespace jex
