#include <jex_jexc.hpp>

#include <jex_compiler.hpp>
#include <jex_environment.hpp>
#include <jex_builtins.hpp>

#include <fstream>
#include <streambuf>

using namespace jex;

int main(int /*argc*/, char *argv[]) {
    JexcCmdParser parser;
    if (!parser.evaluate(argv, std::cerr)) {
        return -1;
    }
    // Read file.
    std::ifstream fileStream(parser.d_fileName.value());
    std::string source((std::istreambuf_iterator<char>(fileStream)),
                        std::istreambuf_iterator<char>());
    if (!fileStream.good()) {
        std::cerr << "Error: Couldn't read " << parser.d_fileName.value() << ".\n";
        return -1;
    }
    if (parser.d_printIR) {
        Environment env;
        env.addModule(BuiltInsModule());
        Compiler::printIR(std::cout, env, source, parser.d_optLevel, parser.d_useIntrinsics, parser.d_enableConstFolding);
    }
    return 0;
}
