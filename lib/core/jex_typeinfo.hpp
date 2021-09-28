#pragma once

#include <jex_base.hpp>

#include <cassert>
#include <functional>
#include <string>

namespace llvm {
class LLVMContext;
class Type;
} // namespace llvm

namespace jex {
class TypeInfo;

enum class TypeKind {
    Unresolved,
    Function,
    Value, // trivial & trivially destructible
    Complex
};

struct LifetimeFcts {
    using DtorT = void(*)(void*);
    using CopyCtorT = void(*)(void* /*target*/, const void* /*source*/);
    using MoveCtorT = void(*)(void* /*target*/, void* /*source*/);

    DtorT destructor = nullptr;
    CopyCtorT copyConstructor = nullptr;
    MoveCtorT moveConstructor = nullptr;
};

/**
 * Represents a unique ID for a given type in the type system.
 * The ID also provides direct access to the referenced TypeInfo.
 */
class TypeInfoId {
    friend class TypeInfo;
    friend class TypeSystem;
    friend struct std::hash<TypeInfoId>;
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

    const TypeInfo& operator*() const {
        return *d_ptr;
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
public:
    using CreateTypeFct = std::function<llvm::Type* (llvm::LLVMContext&)>;
private:
    TypeKind d_typeId;
    std::string d_name;
    size_t d_size;
    size_t d_alignment;
    // Function to create the LLVM type; May be null;
    CreateTypeFct d_createType;
    // Functions to manage lifetime of object.
    LifetimeFcts d_lifetimeFcts;

public:
    TypeInfo(TypeKind typeId, std::string name, size_t size, size_t alignment,
             CreateTypeFct createType, LifetimeFcts lifetimeFcts = {})
    : d_typeId(typeId)
    , d_name(std::move(name))
    , d_size(size)
    , d_alignment(alignment)
    , d_createType(createType)
    , d_lifetimeFcts(lifetimeFcts) {
    }

    TypeInfoId id() const {
        return TypeInfoId(this);
    }

    const std::string& name() const {
        return d_name;
    }

    size_t size() const {
        return d_size;
    }

    size_t alignment() const {
        return d_alignment;
    }

    TypeKind kind() const {
        return d_typeId;
    }

    const CreateTypeFct& createTypeFct() const {
        return d_createType;
    }

    const LifetimeFcts& lifetimeFcts() const {
        return d_lifetimeFcts;
    }
};

} // namespace jex


namespace std {

template<> struct hash<jex::TypeInfoId> {
    std::size_t operator()(jex::TypeInfoId const& id) const noexcept {
        return hash<const void*>{}(id.d_ptr);
    }
};

} // namespace std
