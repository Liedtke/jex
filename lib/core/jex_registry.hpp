#pragma once

#include <jex_environment.hpp>
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

template<typename ArgRet, typename... ArgT>
struct FctDesc {
    using FctType = void(*)(typename ArgRet::retType, typename ArgT::type...);
    using IntrinsicFct = FctInfo::IntrinsicFct;

    std::string name;
    FctType fctPtr;
    IntrinsicFct intrinsicFct;
    std::string retTypeName;
    std::vector<std::string> argTypeNames;

    template<typename... T, size_t... I>
    static void wrapperInner(void(*fct)(typename ArgRet::retType, T...),
                             void** args,
                             std::index_sequence<I...>) {
        fct(reinterpret_cast<typename ArgRet::retType>(args[0]),
            *reinterpret_cast<typename ArgT::type*>(args[I+1])...);
    }

    static void wrapper(void* fct, void** args) {
        // add return value "reinterpret_cast<typename ArgRet::retType>(args[0])"
        wrapperInner(reinterpret_cast<FctType>(fct), args, std::index_sequence_for<ArgT...>());
    }

    FctDesc(std::string name, FctType fctPtr, IntrinsicFct intrinsicFct = nullptr)
    : name(std::move(name))
    , fctPtr(fctPtr)
    , intrinsicFct(std::move(intrinsicFct))
    , retTypeName(ArgRet::name)
    , argTypeNames{ArgT::name...} {
    }
};

class Registry {
    TypeSystem& d_types;
    FctLibrary& d_fcts;

public:
    Registry(Environment& env)
    : d_types(env.types())
    , d_fcts(env.fctLib()) {
    }

    template <typename ArgT>
    void registerType(TypeInfo::CreateTypeFct fct = nullptr) {
        d_types.registerType(ArgT::kind, ArgT::name, sizeof(typename ArgT::type), fct);
    }

    template <typename ...T>
    void registerFct(FctDesc<T...>&& desc) {
        // Resolve return and parameter types.
        TypeInfoId retTypeInfo = d_types.getType(desc.retTypeName);
        std::vector<TypeInfoId> paramTypeInfos;
        std::transform(desc.argTypeNames.begin(), desc.argTypeNames.end(),
            std::back_inserter(paramTypeInfos),
            [this](const std::string& name) { return d_types.getType(name); });
        // Add function to function library.
        d_fcts.registerFct(FctInfo(desc.name, reinterpret_cast<void*>(desc.fctPtr),
                           FctDesc<T...>::wrapper, retTypeInfo, paramTypeInfos, desc.intrinsicFct));
    }
};

class Module {
public:
    virtual ~Module() = default;
    virtual void registerTypes(Registry& registry) const = 0;
    virtual void registerFcts(Registry& regitsry) const = 0;
};

} // namespace jex
