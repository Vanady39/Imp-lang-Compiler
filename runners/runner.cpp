#include "runners/runner.h"

#include "codegen/generator.h"
#include "runners/ast_printers/ast_printer.h"
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "semantic/checker/checker.h"
#include "semantic/optimizer/optimizer.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <vector>

using namespace std;

int runSourceFile(const string& path, const string& output, OutputKind kind) {
    ifstream input(path);
    if (!input.is_open()) {
        cout << "Cannot open file" << endl;
        return 1;
    }

    string source = "";
    string line;

    while (getline(input, line)) {
        source += line;
        source += '\n';
    }

    unique_ptr<ILexer> lexer = makeLexer(source);
    vector<Token> tokens = lexer->GetBuffer(path);

    unique_ptr<IParser> parser = makeParser(tokens);
    unique_ptr<Program> program = parser->parseProgram();

    const vector<string>& lexerErrors = lexer->GetErrors();
    const vector<string>& parserErrors = parser->GetErrors();

    for (size_t i = 0; i < lexerErrors.size(); i++) {
        cout << lexerErrors[i] << endl;
    }
    for (size_t i = 0; i < parserErrors.size(); i++) {
        cout << parserErrors[i] << endl;
    }

    if (lexerErrors.size() > 0 || parserErrors.size() > 0) {
        return 1;
    }

    unique_ptr<IChecker> checker = makeChecker();
    checker->check(*program);
    const vector<string>& semanticErrors = checker->get_errors();
    for (size_t i = 0; i < semanticErrors.size(); i++) {
        cout << semanticErrors[i] << endl;
    }

    if (!semanticErrors.empty()) {
        return 1;
    }

    unique_ptr<IOptimizer> optimizer = makeOptimizer();
    optimizer->optimize(*program);

    if (kind == OutputKind::Ast) {
        printProgram(program.get());
        return 0;
    }

    try {
        error_code filesystemError;
        if (filesystem::equivalent(path, output, filesystemError)) {
            cerr << "Output path would overwrite the source file\n";
            return 1;
        }
        const auto ir = generateLlvmIr(*program);
        const auto bitcode = makeLlvmBitcode(ir);
        const auto& contents = kind == OutputKind::Bitcode ? bitcode : ir;
        ofstream target(output, ios::binary);
        target.write(contents.data(), static_cast<streamsize>(contents.size()));
        target.close();
        if (!target) {
            cerr << "Cannot write output file: " << output << '\n';
            return 1;
        }
        cout << "Wrote " << (kind == OutputKind::Bitcode ? "LLVM bitcode" : "LLVM IR")
             << " to " << output << '\n';
        return 0;
    } catch (const exception& error) {
        cerr << error.what() << '\n';
        return 1;
    }
}
