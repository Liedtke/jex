#pragma once

namespace jex {

// For now, the type system is an enum.
enum class Type {
    Unresolved,
    Integer,
    Float,
    Bool,
    String,
    Function
};

} // namespace jex
