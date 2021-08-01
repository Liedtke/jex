#include <jex_builtins.hpp>

#include <cassert>

namespace jex {

namespace {

template <typename T>
void add(T* res, T a, T b) {
    assert(res != nullptr);
    *res = a + b;
}

} // anonymous namespace

void BuiltInsModule::registerTypes(Registry& registry) {
    registry.registerType<ArgInteger>();
    registry.registerType<ArgFloat>();
    registry.registerType<ArgBool>();
}

void BuiltInsModule::registerFcts(Registry& registry) {
    registry.registerFct(FctDesc<ArgInteger, ArgInteger, ArgInteger>(add, "add"));
    registry.registerFct(FctDesc<ArgFloat, ArgFloat, ArgFloat>(add, "add"));
}

} // namespace jex
