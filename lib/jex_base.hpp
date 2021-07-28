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

} // namespace jex
