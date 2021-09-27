#pragma once

#include <jex_environment.hpp>

#include <gtest/gtest.h>

// Ensure backwards compatibility for some older gtest versions.
#ifndef INSTANTIATE_TEST_SUITE_P
#define INSTANTIATE_TEST_SUITE_P(...) INSTANTIATE_TEST_CASE_P(__VA_ARGS__)
#endif

namespace jex::test {

static inline void registerBuiltIns(Environment& env) {
    TypeSystem& types = env.types();
    types.registerType(TypeKind::Value, "Bool", 1);
    types.registerType(TypeKind::Value, "Integer", 8);
    types.registerType(TypeKind::Value, "Float", 8);
    types.registerType(TypeKind::Value, "String", 2);
}

}
