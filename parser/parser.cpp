#include "parser/parser_impl.h"

#include <format>

using namespace std;

namespace {

string describe(TokenCode code) {
    switch (code) {
    case tokenIdentifier:
    case tokenIntegerLiteral:
    case tokenRealLiteral:
    case tokenNewline:
    case tokenEOF:
    case tokenUnknown:
        return tokenName(code);
    default:
        return format("'{}'", tokenName(code));
    }
}

string describe(const Token& token) {
    switch (token.code) {
    case tokenIdentifier:
        return format("identifier '{}'", token.ID);
    case tokenIntegerLiteral:
    case tokenRealLiteral:
        return format("{} {}", tokenName(token.code), token.ID);
    default:
        return describe(token.code);
    }
}

struct SyntaxError {};

} // namespace

// Инфраструктура: токены, ошибки, восстановление после ошибок.

Parser::Parser(vector<Token> source) : tokens(std::move(source)) {
    if (tokens.empty() || tokens.back().code != tokenEOF) {
        int line = tokens.empty() ? 1 : tokens.back().span.lineNum;
        tokens.push_back({ { line, 0, 0 }, tokenEOF, 0, 0, "" });
    }
}

const vector<string>& Parser::GetErrors() const {
    return errors;
}

const Token& Parser::current() const {
    return tokens[position];
}

bool Parser::check(TokenCode code) const {
    return current().code == code;
}

Token Parser::advance() {
    Token token = current();
    if (token.code != tokenEOF) {
        position++;
    }
    return token;
}

bool Parser::match(TokenCode code) {
    if (!check(code)) return false;
    advance();
    return true;
}

Token Parser::expect(TokenCode code) {
    if (!check(code)) {
        error(describe(code));
    }
    return advance();
}

bool Parser::isSeparator() const {
    return check(tokenNewline) || check(tokenSemicolon);
}

void Parser::skipSeparators() {
    while (isSeparator()) {
        advance();
    }
}

void Parser::report(const string& message) {
    const Token& token = current();
    if (token.code != tokenUnknown && token.span.lineNum != lastErrorLine) {
        errors.push_back(format("{} on line {} column {}",
            message, token.span.lineNum, token.span.posBegin));
    }
    lastErrorLine = token.span.lineNum;
}

void Parser::error(const string& expected) {
    report(format("Expected {} but found {}", expected, describe(current())));
    throw SyntaxError{};
}

void Parser::synchronize() {
    while (!check(tokenEOF) && !isSeparator() && !check(tokenEnd) && !check(tokenElse)) {
        advance();
    }
}

void Parser::parseSequence(const function<bool()>& atEnd, const function<void()>& parseItem) {
    skipSeparators();
    while (!atEnd() && !check(tokenEOF)) {
        size_t start = position;
        try {
            parseItem();
            if (!isSeparator() && !atEnd() && !check(tokenEOF)) {
                error("end of line or ';'");
            }
        } catch (const SyntaxError&) {
            synchronize();
            if (position == start) {
                advance();
            }
        }
        skipSeparators();
    }
}

// Программа и декларации.

unique_ptr<Program> Parser::parseProgram() {
    Span span = current().span;
    vector<DeclarationPtr> declarations;
    parseSequence(
        [this] { return check(tokenEOF); },
        [&] { declarations.push_back(parseDeclaration()); });
    return make_unique<Program>(span, std::move(declarations));
}

DeclarationPtr Parser::parseDeclaration() {
    switch (current().code) {
    case tokenVar:
        return parseVariableDeclaration();
    case tokenType:
        return parseTypeDeclaration();
    case tokenRoutine:
        return parseRoutineDeclaration();
    default:
        error("declaration");
    }
    return nullptr;
}

unique_ptr<VariableDeclaration> Parser::parseVariableDeclaration() {
    Token start = expect(tokenVar);
    Token name = expect(tokenIdentifier);
    TypePtr type;
    ExpressionPtr initializer;
    if (match(tokenColon)) {
        type = parseType();
        if (match(tokenIs)) {
            initializer = parseExpression();
        }
    } else if (match(tokenIs)) {
        initializer = parseExpression();
    } else {
        error("':' or 'is'");
    }
    return make_unique<VariableDeclaration>(start.span, name.ID, std::move(type), std::move(initializer));
}

unique_ptr<TypeDeclaration> Parser::parseTypeDeclaration() {
    Token start = expect(tokenType);
    Token name = expect(tokenIdentifier);
    expect(tokenIs);
    TypePtr type = parseType();
    return make_unique<TypeDeclaration>(start.span, name.ID, std::move(type));
}

unique_ptr<RoutineDeclaration> Parser::parseRoutineDeclaration() {
    Token start = expect(tokenRoutine);
    Token name = expect(tokenIdentifier);
    expect(tokenLeftParen);
    vector<unique_ptr<ParameterDeclaration>> parameters;
    if (!check(tokenRightParen)) {
        do {
            parameters.push_back(parseParameterDeclaration());
        } while (match(tokenComma));
    }
    expect(tokenRightParen);

    TypePtr returnType;
    if (match(tokenColon)) {
        returnType = parseType();
    }

    BodyPtr body;
    ExpressionPtr expressionBody;
    if (match(tokenIs)) {
        body = parseBody();
        expect(tokenEnd);
    } else if (match(tokenArrow)) {
        expressionBody = parseExpression();
    }
    return make_unique<RoutineDeclaration>(start.span, name.ID, std::move(parameters),
        std::move(returnType), std::move(body), std::move(expressionBody));
}

unique_ptr<ParameterDeclaration> Parser::parseParameterDeclaration() {
    Token name = expect(tokenIdentifier);
    expect(tokenColon);
    TypePtr type = parseType();
    return make_unique<ParameterDeclaration>(name.span, name.ID, std::move(type));
}

// Типы.

TypePtr Parser::parseType() {
    switch (current().code) {
    case tokenInteger:
    case tokenReal:
    case tokenBoolean: {
        Token token = advance();
        return make_unique<PrimitiveType>(token.span, token.code);
    }
    case tokenIdentifier: {
        Token token = advance();
        return make_unique<NamedType>(token.span, token.ID);
    }
    case tokenArray:
        return parseArrayType();
    case tokenRecord:
        return parseRecordType();
    default:
        error("type");
    }
    return nullptr;
}

TypePtr Parser::parseArrayType() {
    Token start = expect(tokenArray);
    expect(tokenLeftBracket);
    ExpressionPtr size;
    if (!check(tokenRightBracket)) {
        size = parseExpression();
    }
    expect(tokenRightBracket);
    TypePtr elementType = parseType();
    return make_unique<ArrayType>(start.span, std::move(size), std::move(elementType));
}

TypePtr Parser::parseRecordType() {
    Token start = expect(tokenRecord);
    vector<unique_ptr<VariableDeclaration>> fields;
    parseSequence(
        [this] { return check(tokenEnd); },
        [&] { fields.push_back(parseVariableDeclaration()); });
    expect(tokenEnd);
    return make_unique<RecordType>(start.span, std::move(fields));
}

unique_ptr<IParser> makeParser(vector<Token> tokens) {
    return make_unique<Parser>(std::move(tokens));
}
