#ifndef PARSER_H
#define PARSER_H

#include "lexer/lexer.h"
#include "parser/ast.h"

#include <memory>
#include <string>
#include <vector>

using namespace std;

class IParser {
public:
    virtual ~IParser() = default;
    virtual unique_ptr<Program> parseProgram() = 0;
    virtual const vector<string>& GetErrors() const = 0;
};

unique_ptr<IParser> makeParser(vector<Token> tokens);

#endif
