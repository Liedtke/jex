#pragma once

#include <jex_compiler.hpp>

#include <cassert>
#include <functional>
#include <iostream>
#include <map>
#include <optional>

namespace jex {

namespace cmdUtils {

template <typename T>
static void parse(T* /*result*/, const std::string& /*in*/) {
    static_assert(!sizeof(T), "Missing specialization"); // LCOV_EXCL_LINE
}

template <>
inline void parse<int>(int* result, const std::string& in) {
    size_t pos;
    *result = std::stoi(in, &pos);
}

} // namespace cmdUtils

struct CmdOption {
    using Callback = std::function<void(const std::string&)>;
    char letter;
    std::string name;
    std::string info;
    bool hasParam;
    Callback callback;
};

class CmdParser {
public:
    std::map<std::string, CmdOption> d_options;
    std::unordered_map<char, const CmdOption*> d_optionsByLetter;

    void addOption(char letter, const std::string& name, std::string info, bool hasParam, CmdOption::Callback callback) {
        auto [iter, inserted] = d_options.emplace(name, CmdOption{letter, name, std::move(info), hasParam, std::move(callback)});
        assert(inserted && "Duplicate option name");
        inserted = d_optionsByLetter.emplace(letter, &iter->second).second;
        assert(inserted && "Duplicate option letter");
    }

    const CmdOption* getOption(const std::string& name) const {
        if (name[0] != '-') {
            return nullptr;
        }
        if (name[1] == '-') {
            auto iter = d_options.find(name.substr(2));
            return iter != d_options.end() ? &iter->second : nullptr;
        }
        auto iter = d_optionsByLetter.find(name[1]);
        return iter != d_optionsByLetter.end() ? iter->second : nullptr;
    }

    bool evaluate(char** argv, std::ostream& err) const {
        assert(*argv != nullptr);
        ++argv; // Skip program name.
        while(*argv != nullptr) {
            const CmdOption* option = getOption(*argv);
            if (!option) {
                err << "Error: Invalid option '" << *argv << "'.\n";
                return false;
            }
            const char* param = "";
            if (option->hasParam) {
                param = *++argv;
                if (param == nullptr) {
                    err << "Error: Missing parameter for option '" << option->name << "'.\n";
                    return false;
                }
            }
            try {
                option->callback(param);
            } catch (const std::logic_error& exc) {
                err << "Error while parsing option '" << option->name << "': " << exc.what() << ".\n";
                return false;
            }
            ++argv;
        }
        return true;
    }
};

struct JexcCmdParser {
    CmdParser d_parser;
    OptLevel d_optLevel = OptLevel::O0;
    bool d_useIntrinsics = true;
    bool d_enableConstFolding = true;
    std::optional<std::string> d_fileName;
    std::optional<std::string> d_outFileName;
    bool d_printIR = false;
public:
    JexcCmdParser() {
        d_parser.addOption('O', "opt-level", "Optimization level to be used. Supported levels are O0 - O2.", true,
            [this](const std::string& in) {
                int level;
                cmdUtils::parse(&level, in);
                if (level < 0 || level > 2) {
                    throw std::logic_error("Invalid range");
                }
                d_optLevel = static_cast<OptLevel>(level);
            });
        d_parser.addOption('i', "no-intrinsics", "Disable intrinsics.", false,
           [this](const std::string& /*in*/) {
               d_useIntrinsics = false;
           });
        d_parser.addOption('c', "no-const-folding", "Disable constant folding.", false,
           [this](const std::string& /*in*/) {
               d_enableConstFolding = false;
           });
        d_parser.addOption('f', "input-file", "File containing the expressions to be parsed.", true,
           [this](const std::string& in) {
               d_fileName.emplace(in);
           });
        d_parser.addOption('l', "emit-llvm", "Print LLVM IR code.", false,
           [this](const std::string& /*in*/) {
               d_printIR = true;
           });
        d_parser.addOption('o', "output-file", "Write the output to a file.", true,
            [this](const std::string& in ) {
                d_outFileName.emplace(in);
            });
        d_parser.addOption('g', "debug-info", "Enable debug info (currently not supported).", false,
            [](const std::string& /*in*/ ) {});
        d_parser.addOption('S', "compile-only", "Currently not supported.", false,
            [](const std::string& /*in*/ ) {});
    }
    bool evaluate(char** argv, std::ostream& err) {
        bool success = d_parser.evaluate(argv, err);
        if (!success) {
            return false;
        }
        if (!d_fileName.has_value()) {
            err << "Missing file name.\n";
            return false;
        }
        return true;
    }
};

} // namespace jex
