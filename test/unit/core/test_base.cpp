#include <test_base.hpp>

namespace jex::test {

void add(int64_t* res, int64_t a, int64_t b) {
    *res = a + b;
}

void mul(int64_t* res, int64_t a, int64_t b) {
    *res = a * b;
}

void equal(bool* res, int64_t a, int64_t b) {
    *res = a == b;
}

void unot(bool* res, bool in) {
    *res = !in;
}

void uminus(int64_t* res, int64_t a) {
    *res = -a;
}

static void substr(std::string* res, const std::string* in, int64_t pos, int64_t count) {
    new (res) std::string(in->substr(pos, count));
}

void TestModule::registerTypes(Registry& registry) const {
    registry.registerType<ArgBool>();
    registry.registerType<ArgInteger>();
    registry.registerType<ArgFloat>();
    registry.registerType<ArgString>();
}

void TestModule::registerFcts(Registry& registry) const {
    registry.registerFct(FctDesc<ArgInteger, ArgInteger, ArgInteger>("operator_add", add, NO_INTRINSIC, FctFlags::Pure));
    registry.registerFct(FctDesc<ArgInteger, ArgInteger, ArgInteger>("operator_mul", add, NO_INTRINSIC, FctFlags::None));
    registry.registerFct(FctDesc<ArgBool, ArgInteger, ArgInteger>("operator_eq", equal));
    registry.registerFct(FctDesc<ArgString, ArgString, ArgInteger, ArgInteger>("substr", substr, NO_INTRINSIC, FctFlags::Pure));
    registry.registerFct(FctDesc<ArgBool, ArgBool>("operator_not", unot, NO_INTRINSIC, FctFlags::Pure));
    registry.registerFct(FctDesc<ArgInteger, ArgInteger>("operator_uminus", uminus, NO_INTRINSIC, FctFlags::None));
}

}
