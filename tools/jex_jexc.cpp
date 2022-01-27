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
    // Open output stream.
    std::ostream* outStream = &std::cout;
    std::unique_ptr<std::ofstream> outFileStream;
    if (parser.d_outFileName) {
        outFileStream = std::make_unique<std::ofstream>(parser.d_outFileName.value());
        if (!outFileStream->good()) {
            std::cerr << "Error: Couldn't write to " << parser.d_outFileName.value() << ".\n";
            return -1;
        }
        outStream = outFileStream.get();
    }
    // Print IR.
    if (parser.d_printIR) {
        Environment env;
        env.addModule(BuiltInsModule());
        try {
            Compiler::printIR(*outStream, env, source, parser.d_optLevel, parser.d_useIntrinsics, parser.d_enableConstFolding);
        } catch (std::runtime_error& err) {
            std::cerr << err.what();
            return -1;
        }
    }
    return 0;
}
