#include <jex_typesystem.hpp>

#include <jex_errorhandling.hpp>

namespace jex {

TypeSystem::TypeSystem()
: d_types()
, d_typesByName()
, d_unresolved(TypeInfoId(&d_types.emplace_back(TypeKind::Unresolved, "_Unresolved", 0, 0, nullptr))) {
}

TypeInfoId TypeSystem::getTypeOrUnresolved(std::string_view name) const {
    const auto iter = d_typesByName.find(name);
    if (iter == d_typesByName.end()) {
        return d_unresolved;
    }
    return iter->second;
}

TypeInfoId TypeSystem::getType(std::string_view name) const {
    TypeInfoId id = getTypeOrUnresolved(name);
    if (id == d_unresolved) {
        throw InternalError("Type '" + std::string(name) + "' does not exist");
    }
    return id;
}

TypeInfoId TypeSystem::registerType(TypeKind typeId, std::string name, size_t size, size_t alignment,
                                    const TypeInfo::CreateTypeFct& createTypeFct,
                                    bool isZeroInitialized,
                                    TypeInfo::CallConv callConv) {
    if (d_typesByName.find(name) != d_typesByName.end()) {
        throw InternalError("Duplicate type registration for '" + name + "'");
    }
    TypeInfoId id(&d_types.emplace_back(typeId, std::move(name), size, alignment, createTypeFct, isZeroInitialized, callConv));
    auto res = d_typesByName.emplace(id.get().name(), id);
    assert(res.second); (void) res;
    return id;
}

} // namespace jex
