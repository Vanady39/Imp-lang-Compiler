#ifndef PARSER_IMPL_H
#define PARSER_IMPL_H

#include "parser/parser.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

using namespace std;

// Внутренний заголовок парсера: определение класса Parser, общее для
// parser.cpp, parser_stmt.cpp и parser_expr.cpp. Наружу не торчит.
class Parser : public IParser {
    vector<Token> tokens;
    size_t position = 0;
    vector<string> errors;
    int lastErrorLine = -1;

public:
    Parser(vector<Token> source);

    const vector<string>& GetErrors() const override;
    unique_ptr<Program> parseProgram() override;

private:
    // Инфраструктура: токены, ошибки, восстановление после ошибок.
    const Token& current() const;
    bool check(TokenCode code) const;
    Token advance();
    bool match(TokenCode code);
    Token expect(TokenCode code);
    bool isSeparator() const;
    void skipSeparators();
    void report(const string& message);
    void error(const string& expected);
    void synchronize();
    void parseSequence(const function<bool()>& atEnd, const function<void()>& parseItem);

    // Декларации и типы.
    DeclarationPtr parseDeclaration();
    unique_ptr<VariableDeclaration> parseVariableDeclaration();
    unique_ptr<TypeDeclaration> parseTypeDeclaration();
    unique_ptr<RoutineDeclaration> parseRoutineDeclaration();
    unique_ptr<ParameterDeclaration> parseParameterDeclaration();
    TypePtr parseType();
    TypePtr parseArrayType();
    TypePtr parseRecordType();

    // Операторы.
    BodyPtr parseBody();
    NodePtr parseBodyItem();
    StatementPtr parseAssignmentOrCall();
    StatementPtr parseWhileLoop();
    StatementPtr parseForLoop();
    StatementPtr parseIfStatement();
    StatementPtr parsePrintStatement();
    StatementPtr parseReturnStatement();

    // Выражения.
    ExpressionPtr parseExpression();
    ExpressionPtr parseBinary(int minPrecedence);
    ExpressionPtr parsePrimary();
    ExpressionPtr parseLiteral();
    ExpressionPtr parseAccessors(ExpressionPtr base);
    unique_ptr<RoutineCall> parseRoutineCall(const Token& name);
};

#endif
