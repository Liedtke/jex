#include <jex_builtins.hpp>

namespace jex {

namespace {

void add(int64_t* res, int64_t a, int64_t b) {
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
}

} // namespace jex
