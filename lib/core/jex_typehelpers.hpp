#pragma once

namespace jex {

// T* return type could also be replaced with decltype(Op()(T(), T())) if desired.
// However, T* ensures consistency between return type and type of inputs.
template <typename Op, typename T>
void op(T* res, T a, T b) {
    assert(res != nullptr);
    new(res) T(Op()(a, b));
}

template <typename Op, typename T>
void op(T* res, const T* a, const T* b) {
    assert(res != nullptr);
    new(res) T(Op()(*a, *b));
}

template <typename Op, typename T>
void cmpPtr(bool* res, const T* a, const T* b) {
    assert(res != nullptr);
    *res = Op()(*a, *b);
}

template <typename Op, typename T>
void cmp(bool* res, T a, T b) {
    assert(res != nullptr);
    *res = Op()(a, b);
}

template <typename Op, typename T>
void unaryOp(T* res, T arg) {
    assert(res != nullptr);
    new(res) T(Op()(arg));
}

} // namespace jex
