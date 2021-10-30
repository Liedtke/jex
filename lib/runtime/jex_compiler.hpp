# pragma once

#include <jex_base.hpp>
// TODO: Needed for OptLevel, consider moving somewhere else.
#include <jex_codegen.hpp>

namespace jex {

class CompileResult;
class Environment;

class Compiler {
public:
    Compiler() = delete;

    static CompileResult compile(const Environment& env,
                                 const std::string& source,
                                 OptLevel optLevel = OptLevel::O2,
                                 bool useIntrinsics = true,
                                 bool enableConstantFolding = true);

    static void printIR(std::ostream& out,
                        const Environment& env,
                        const std::string& source,
                        OptLevel optLevel = OptLevel::O2,
                        bool useIntrinsics = true,
                        bool enableConstantFolding = true);
};

} // namespace jex
