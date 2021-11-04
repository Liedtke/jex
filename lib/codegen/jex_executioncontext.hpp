#pragma once

#include <jex_base.hpp>

#include <cstddef>
#include <memory>

namespace jex {

class CompileResult;

class ExecutionContext : NoCopy {
    using LifetimeFct = void(*)(void*);
    const LifetimeFct d_dtor;
    const size_t d_size;
    // Compiler extension: Zero-length-array. (non-standard C++)
    // Stores the actual data of the execution context.
    alignas(std::max_align_t) char d_data[0];

    ExecutionContext(const CompileResult& compiled);

    void* operator new(size_t objectSize, const CompileResult& compiled);

public:
    ~ExecutionContext();

    // Overload operator delete without size argument. Otherwise ASAN complains
    // about mismatching sizes (as operator new modifies the allocation size).
    void operator delete(void* ptr) noexcept { // NOLINT
        ::operator delete(ptr);
    }

    static std::unique_ptr<ExecutionContext> create(const CompileResult& compiled);

    char* getDataPtr() {
        return d_data;
    }
};

} // namespace jex
