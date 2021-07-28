#pragma once

#include <string>

namespace jex {

enum class TypeId {
    Unresolved,
    Integer,
    Float,
    Bool,
    String,
    Function,
    Complex // Anything non built-in
};

class TypeInfo {
    TypeId d_typeId;
    std::string d_name;

public:
    TypeInfo(TypeId typeId, std::string name)
    : d_typeId(typeId)
    , d_name(std::move(name)) {
    }
};

} // namespace jex
