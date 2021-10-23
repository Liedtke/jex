#pragma once

#include <jex_fctinfo.hpp>

#include <memory>
#include <unordered_map>

namespace jex {

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

    void* getPtr() {
        return valuePtr.get();
    }

    const void* getPtr() const {
        return valuePtr.get();
    }

    static Constant allocate(size_t size) {
        return Constant{std::make_unique<uint8_t[]>(size), nullptr};
    }

    template<typename T>
    static Constant create(T value, Dtor dtor = nullptr) {
        Constant constant = allocate(sizeof(T));
        new (constant.getPtr()) T(std::move(value));
        constant.dtor = dtor;
        return constant;
    }
};

/**
 * Container to store constants during the lifetime of the compiled program.
 */
class ConstantStore {
    std::unordered_map<std::string, Constant> d_constants;

public:
    ConstantStore() {
    }

    template <typename T, typename... Args>
    const T* emplace(std::string name, const FctInfo& dtor, Args&&... args) {
        auto[iter, inserted] = d_constants.emplace(std::move(name), Constant::allocate(sizeof(T)));
        assert(inserted && "constant name must be unique");
        T* ptr = reinterpret_cast<T*>(iter->second.valuePtr.get());
        new (ptr) T(std::forward<Args>(args)...);
        // Set destructor after the element has been constructed to prevent calling the
        // destructor if the construction failed.
        iter->second.dtor = reinterpret_cast<Constant::Dtor>(dtor.d_fctPtr);
        return ptr;
    }

    void insert(std::string name, Constant constant) {
        [[maybe_unused]] auto[iter, inserted] = d_constants.emplace(name, std::move(constant));
        assert(inserted && "constant name must be unique");
    }

    auto begin() {
        return d_constants.begin();
    }
    auto end() {
        return d_constants.end();
    }

    const Constant& constantByName(const std::string& name) {
        return d_constants.at(name);
    }
};

} // namesapce jex
