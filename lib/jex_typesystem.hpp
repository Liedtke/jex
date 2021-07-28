#pragma once

#include <jex_typeinfo.hpp>

#include <deque>
#include <unordered_map>

namespace jex {
class CompileEnv;
class TypeInfo;

class TypeSystem {
    CompileEnv& d_env;
    std::deque<TypeInfo> d_types;
    std::unordered_map<std::string_view, TypeInfoId> d_typesByName;
    const TypeInfoId d_unresolved;
public:
    TypeSystem(CompileEnv& env);

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
    TypeInfoId registerType(TypeId typeId, std::string name);

    TypeInfoId unresolved() const {
        return d_unresolved;
    }
};

} // namespace jex
