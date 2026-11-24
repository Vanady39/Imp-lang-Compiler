#include "runners/runner.h"

#include <filesystem>
#include <iostream>
#include <string_view>

using namespace std;

int main(int argc, char** argv) {
    string source;
    string output;
    OutputKind kind = OutputKind::Bitcode;
    const auto usage = [] {
        cerr << "Usage: compiler <source-file> [-o <output>] [--emit-llvm | --dump-ast]\n";
        return 1;
    };
    for (int i = 1; i < argc; ++i) {
        const string_view arg = argv[i];
        if (arg == "-o" && output.empty() && i + 1 < argc) {
            output = argv[++i];
        } else if (arg == "--emit-llvm" && kind == OutputKind::Bitcode) {
            kind = OutputKind::LlvmIr;
        } else if (arg == "--dump-ast" && kind == OutputKind::Bitcode) {
            kind = OutputKind::Ast;
        } else if (!arg.starts_with('-') && source.empty()) {
            source = arg;
        } else {
            return usage();
        }
    }
    if (source.empty() || (kind == OutputKind::Ast && !output.empty())) return usage();
    if (output.empty() && kind != OutputKind::Ast) {
        auto path = filesystem::path(source);
        const char* extension = ".bc";
        if (kind == OutputKind::LlvmIr) {
            extension = ".ll";
        }
        output = path.replace_extension(extension).string();
    }
    return runSourceFile(source, output, kind);
}
