#pragma once

#include <jex_base.hpp>

#include <cassert>
#include <string>

namespace jex {
class TypeInfo;

enum class TypeId {
    Unresolved,
    Integer,
    Float,
    Bool,
    String,
    Function,
    Complex // Anything non-built-in
};

/**
 * Represents a unique ID for a given type in the type system.
 * The ID also provides direct access to the referenced TypeInfo.
 */
class TypeInfoId {
    friend class TypeInfo;
    friend class TypeSystem;
    const TypeInfo* d_ptr;

    explicit TypeInfoId(const TypeInfo* ptr)
    : d_ptr(ptr) {
        assert(ptr != nullptr);
    }
public:
    const TypeInfo& get() const {
        return *d_ptr;
    }

    const TypeInfo* operator->() const {
        return d_ptr;
    }

    bool operator==(const TypeInfoId& other) const {
        return d_ptr == other.d_ptr;
    }
    bool operator!=(const TypeInfoId& other) const {
        return !(*this == other);
    }
};


/**
 * Represents a type in the type system and defines its properties.
 */
class TypeInfo : NoCopy {
    TypeId d_typeId;
    std::string d_name;

public:
    TypeInfo(TypeId typeId, std::string name)
    : d_typeId(typeId)
    , d_name(std::move(name)) {
    }

    TypeInfoId id() const {
        return TypeInfoId(this);
    }

    std::string_view name() const {
        return d_name;
    }

    TypeId kind() const {
        return d_typeId;
    }
};

} // namespace jex
