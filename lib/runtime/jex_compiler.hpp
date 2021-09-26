# pragma once

#include <jex_base.hpp>
// TODO: Needed for OptLevel, consider moving somewhere else.
#include <jex_codegen.hpp>

namespace jex {

class CompileResult;
class Environment;

class Compiler {
    Compiler() = delete;
public:
    static CompileResult compile(Environment& env,
                                 const std::string& source,
                                 OptLevel optLevel = OptLevel::O2,
                                 bool useIntrinsics = true);
};

} // namespace jex
