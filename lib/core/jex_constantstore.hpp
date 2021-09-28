#pragma once

#include <memory>
#include <vector>

namespace jex {

/**
 * Container to store constants during the lifetime of the compiled program.
 */
class ConstantStore {
    std::vector<std::unique_ptr<uint8_t[]>> d_constants;
public:
    ConstantStore() {
    }

    template <typename T, typename... Args>
    const T* emplace(Args&&... args) {
        d_constants.push_back(std::make_unique<uint8_t[]>(sizeof(T)));
        T* ptr = reinterpret_cast<T*>(d_constants.back().get());
        new (ptr) T(std::forward<Args>(args)...);
        return ptr;
    }
};

} // namesapce jex
