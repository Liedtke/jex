#pragma once

#include <jex_base.hpp>
#include <jex_fctlibrary.hpp>
#include <jex_typesystem.hpp>

namespace jex {
class Module;

class Environment : NoCopy {
    TypeSystem d_types;
    FctLibrary d_fctLib;

public:
    Environment()
    : d_types()
    , d_fctLib() {
    }

    void addModule(const Module& module);

    TypeSystem& types() {
        return d_types;
    }
    const TypeSystem& types() const {
        return d_types;
    }

    FctLibrary& fctLib() {
        return d_fctLib;
    }
    const FctLibrary& fctLib() const {
        return d_fctLib;
    }
};

} // namespace jex
