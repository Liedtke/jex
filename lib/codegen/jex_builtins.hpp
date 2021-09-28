#pragma once

#include <jex_registry.hpp>

#include <cstdint>

namespace jex {

static constexpr char IntegerName[] = "Integer";
using ArgInteger = ArgValue<int64_t, IntegerName>;

static constexpr char FloatName[] = "Float";
using ArgFloat = ArgValue<double, FloatName>;

static constexpr char BoolName[] = "Bool";
using ArgBool = ArgValue<bool, BoolName>;

static constexpr char StringName[] = "String";
using ArgString = ArgObject<std::string, StringName>;

/**
 * Defines a module containing the built-in types and functions.
 */
class BuiltInsModule : public Module {
    void registerTypes(Registry& registry) const override;
    void registerFcts(Registry& registry) const override;
};

} // namespace jex
