#include "lexer/lexer.h"

#include <fstream>
#include <iostream>
#include <string>

using namespace std;

int main(int argc, char** argv) {
    if (argc != 2) {
        cout << "Usage: lexer_dump <source-file>" << endl;
        return 1;
    }

    ifstream input(argv[1]);
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

    Lexer lexer(source);
    vector<Token> tokens = lexer.GetBuffer(argv[1]);

    for (int i = 0; i < tokens.size(); i++) {
        Token token = tokens[i];

        cout << "line " << token.span.lineNum
             << ", columns " << token.span.posBegin
             << "-" << token.span.posEnd
             << ": " << tokenName(token.code);

        if (token.code == tokenIdentifier) {
            cout << " (" << token.ID << ")";
        }

        if (token.code == tokenIntegerLiteral) {
            cout << " (" << token.intValue << ")";
        }

        if (token.code == tokenRealLiteral) {
            cout << " (" << token.realValue << ")";
        }

        cout << endl;
    }

    const vector<string>& errors = lexer.GetErrors();

    for (int i = 0; i < errors.size(); i++) {
        cout << errors[i] << endl;
    }

    if (errors.size() > 0) {
        return 1;
    }

    return 0;
}
