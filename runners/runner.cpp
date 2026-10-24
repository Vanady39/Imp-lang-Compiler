#include "runners/runner.h"

#include "runners/ast_printers/ast_printer.h"
#include "lexer/lexer.h"
#include "parser/parser.h"

#include <fstream>
#include <iostream>
#include <memory>
#include <vector>

using namespace std;

int runSourceFile(const string& path) {
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

    printProgram(program.get());
    return 0;
}
