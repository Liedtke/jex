#include <jex_environment.hpp>

#include <jex_registry.hpp>

namespace jex {

void Environment::addModule(const Module& module) {
    Registry registry(*this);
    module.registerTypes(registry);
    module.registerFcts(registry);
}

} // namespace jex
