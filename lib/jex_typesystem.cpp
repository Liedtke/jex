#include <jex_typesystem.hpp>

#include <jex_errorhandling.hpp>

namespace jex {

namespace {

static const struct {
    TypeId id;
    const char* name;
} s_builtIns[] = {
    {TypeId::Integer, "Integer"},
    {TypeId::Float, "Float"},
    {TypeId::Bool, "Bool"},
    {TypeId::String, "String"},
};

} // anonymous namespace

TypeSystem::TypeSystem(CompileEnv& env)
: d_env(env)
, d_types()
, d_typesByName()
, d_unresolved(TypeInfoId(&d_types.emplace_back(TypeId::Unresolved, "_Unresolved"))) {
    // Register built-ins.
    for (const auto& bi : s_builtIns) {
        registerType(bi.id, bi.name);
    }
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

TypeInfoId TypeSystem::registerType(TypeId typeId, std::string name) {
    if (d_typesByName.find(name) != d_typesByName.end()) {
        throw InternalError("Duplicate type registration for '" + name + "'");
    }
    TypeInfoId id(&d_types.emplace_back(typeId, std::move(name)));
    auto res = d_typesByName.emplace(id.get().name(), id);
    assert(res.second); (void) res;
    return id;
}

} // namespace jex
