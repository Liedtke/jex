#pragma once

#include <jex_fctinfo.hpp>

#include <memory>
#include <unordered_map>

namespace jex {

/**
 * Container to store constants during the lifetime of the compiled program.
 */
class ConstantStore {
    class Constant {
    public:
        using Dtor = void(*)(void*);
        std::unique_ptr<uint8_t[]> valuePtr;
        Dtor dtor;

        Constant(Constant&&) = default;
        Constant& operator=(Constant&&) = default;

        ~Constant() {
            // Destruct constant if destructor is set.
            if (valuePtr && dtor != nullptr) {
                dtor(valuePtr.get());
            }
        }
    };

    std::unordered_map<std::string, Constant> d_constants;
public:
    ConstantStore() {
    }

    template <typename T, typename... Args>
    const T* emplace(std::string name, const FctInfo& dtor, Args&&... args) {
        auto[iter, inserted] = d_constants.emplace(std::move(name),
            Constant{std::make_unique<uint8_t[]>(sizeof(T)), nullptr});
        assert(inserted && "constant name not unique");
        T* ptr = reinterpret_cast<T*>(iter->second.valuePtr.get());
        new (ptr) T(std::forward<Args>(args)...);
        // Set destructor after the element has been constructed to prevent calling the
        // destructor if the construction failed.
        iter->second.dtor = reinterpret_cast<Constant::Dtor>(dtor.d_fctPtr);
        return ptr;
    }

    auto begin() {
        return d_constants.begin();
    }
    auto end() {
        return d_constants.end();
    }
};

} // namesapce jex
