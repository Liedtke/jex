#pragma once

#include <jex_fctinfo.hpp>
#include <jex_fctlibrary.hpp>
#include <jex_typesystem.hpp>

#include <algorithm>
#include <iterator>

namespace jex {

template<typename T, const char* _name, TypeId _kind = TypeId::Complex>
struct Arg {
    using type = T;
    using retType = T*;
    static constexpr TypeId kind = _kind;
    static constexpr char const* name = _name;
};

template<typename ArgRet, typename ...ArgT>
struct FctDesc {
    using FctType = void(*)(typename ArgRet::retType, typename ArgT::type...);

    FctType fctPtr;
    std::string name;
    std::string retTypeName;
    std::vector<std::string> argTypeNames;

    FctDesc(FctType fctPtr, std::string name)
    : fctPtr(fctPtr)
    , name(std::move(name))
    , retTypeName(ArgRet::name)
    , argTypeNames{ArgT::name...} {
    }
};

class Registry {
    TypeSystem& d_types;
    FctLibrary& d_fcts;

public:
    Registry(TypeSystem& types, FctLibrary& fcts)
    : d_types(types)
    , d_fcts(fcts) {
    }

    template <typename ArgT>
    void registerType() {
        d_types.registerType(ArgT::kind, ArgT::name);
    }

    template <typename FctDescT>
    void registerFct(FctDescT&& desc) {
        TypeInfoId retTypeInfo = d_types.getType(desc.retTypeName);
        std::vector<TypeInfoId> paramTypeInfos;
        std::transform(desc.argTypeNames.begin(), desc.argTypeNames.end(),
            std::back_inserter(paramTypeInfos),
            [this](const std::string& name) { return d_types.getType(name); });
        d_fcts.registerFct(FctInfo(desc.name, reinterpret_cast<void*>(desc.fctPtr), retTypeInfo, paramTypeInfos));
    }
};

class Module {
public:
    virtual ~Module() = default;
    virtual void registerTypes(Registry& registry);
    virtual void registerFcts(Registry& regitsry);
};

} // namespace jex
