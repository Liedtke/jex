#pragma once

#include <jex_base.hpp>
#include <jex_typeinfo.hpp>

#include <deque>
#include <unordered_map>

namespace jex {
class CompileEnv;
class TypeInfo;

class TypeSystem : NoCopy {
public:
    using TypesByName = std::unordered_map<std::string_view, TypeInfoId>;
private:
    std::deque<TypeInfo> d_types;
    TypesByName d_typesByName;
    const TypeInfoId d_unresolved;
public:
    TypeSystem();

    /**
     * Returns TypeInfoId for given type name. If the type is not registered,
     * returns "_Unresolved" TypeInfoId instead.
     */
    TypeInfoId getTypeOrUnresolved(std::string_view name) const;

    /**
     * Returns TypeInfoId for given type name. If the type is not registered,
     * throws InternalError instead.
     */
    TypeInfoId getType(std::string_view name) const;

    /**
     * Registers a new type with given type kind and name. If a type with the same name has already
     * been registered, throws an InternalError.
     */
    TypeInfoId registerType(TypeId typeId, std::string name, size_t size, TypeInfo::CreateTypeFct createTypeFct = nullptr);

    TypeInfoId unresolved() const {
        return d_unresolved;
    }

    bool isResolved(TypeInfoId id) const {
        return id != d_unresolved;
    }

    TypesByName::const_iterator begin() const {
        return d_typesByName.begin();
    }

    TypesByName::const_iterator end() const {
        return d_typesByName.end();
    }
};

} // namespace jex
