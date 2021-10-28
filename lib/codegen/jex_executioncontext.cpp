#include <jex_executioncontext.hpp>

#include <jex_backend.hpp>

#include <cassert>

namespace jex {

ExecutionContext::ExecutionContext(const CompileResult& compiled)
: d_dtor(reinterpret_cast<LifetimeFct>(compiled.getFctPtr("__destruct_rctx")))
, d_size(compiled.getContextSize()) {
    // Initialize all context variables.
    auto ctor = reinterpret_cast<LifetimeFct>(compiled.getFctPtr("__init_rctx"));
    ctor(getDataPtr());
}

ExecutionContext::~ExecutionContext() {
    // Destruct all context variables.
    d_dtor(getDataPtr());
}

void* ExecutionContext::operator new(size_t objectSize, const CompileResult& compiled) {
    assert(objectSize >= alignof(std::max_align_t));
    assert(objectSize % alignof(std::max_align_t) == 0);
    size_t totalSize = objectSize + compiled.getContextSize();
    return ::operator new(totalSize);
}

std::unique_ptr<ExecutionContext> ExecutionContext::create(const CompileResult& compiled) {
    return std::unique_ptr<ExecutionContext>(new(compiled) ExecutionContext(compiled));
}

} // namespace jex
