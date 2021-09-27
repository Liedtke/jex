#pragma once

#include <jex_registry.hpp>

#include <cstdint>

namespace jex {

static constexpr char IntegerName[] = "Integer";
using ArgInteger = Arg<int64_t, IntegerName, TypeKind::Value>;

static constexpr char FloatName[] = "Float";
using ArgFloat = Arg<double, FloatName, TypeKind::Value>;

static constexpr char BoolName[] = "Bool";
using ArgBool = Arg<bool, BoolName, TypeKind::Value>;

// TODO: Come up with concept for strings which require some allocation / ownership /
// call convention.

/**
 * Defines a module containing the built-in types and functions.
 */
class BuiltInsModule : public Module {
    void registerTypes(Registry& registry) const override;
    void registerFcts(Registry& registry) const override;
};

} // namespace jex
