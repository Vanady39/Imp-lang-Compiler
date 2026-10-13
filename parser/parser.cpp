#include "parser.h"

#include <format>
#include <functional>

using namespace std;

struct SyntaxError {};

static bool isRelationOperator(TokenCode code) {
    return code == tokenLess || code == tokenLessEqual || code == tokenGreater
        || code == tokenGreaterEqual || code == tokenEqual || code == tokenNotEqual;
}

static string describe(TokenCode code) {
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

static string describe(const Token& token) {
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

class Parser : public IParser {
    vector<Token> tokens;
    size_t position = 0;
    vector<string> errors;
    int lastErrorLine = -1;
    bool insideTypedRoutine = false;

public:
    Parser(vector<Token> source) : tokens(std::move(source)) {
        if (tokens.empty() || tokens.back().code != tokenEOF) {
            int line = tokens.empty() ? 1 : tokens.back().span.lineNum;
            tokens.push_back({ { line, 0, 0 }, tokenEOF, 0, 0, "" });
        }
    }

    const vector<string>& GetErrors() const override {
        return errors;
    }
    unique_ptr<Program> parseProgram() override {
        Span span = current().span;
        vector<DeclarationPtr> declarations;
        parseSequence(
            [this] { return check(tokenEOF); },
            [&] { declarations.push_back(parseDeclaration()); });
        return make_unique<Program>(span, std::move(declarations));
    }

private:
    const Token& current() const {
        return tokens[position];
    }

    bool check(TokenCode code) const {
        return current().code == code;
    }

    Token advance() {
        Token token = current();
        if (token.code != tokenEOF) {
            position++;
        }
        return token;
    }

    bool match(TokenCode code) {
        if (!check(code)) return false;
        advance();
        return true;
    }

    Token expect(TokenCode code) {
        if (!check(code)) {
            error(describe(code));
        }
        return advance();
    }

    bool isSeparator() const {
        return check(tokenNewline) || check(tokenSemicolon);
    }

    void skipSeparators() {
        while (isSeparator()) {
            advance();
        }
    }

    void report(const string& message) {
        const Token& token = current();
        if (token.code != tokenUnknown && token.span.lineNum != lastErrorLine) {
            errors.push_back(format("{} on line {} column {}",
                message, token.span.lineNum, token.span.posBegin));
        }
        lastErrorLine = token.span.lineNum;
    }

    [[noreturn]] void error(const string& expected) {
        report(format("Expected {} but found {}", expected, describe(current())));
        throw SyntaxError{};
    }

    void synchronize() {
        while (!check(tokenEOF) && !isSeparator() && !check(tokenEnd) && !check(tokenElse)) {
            advance();
        }
    }

    void parseSequence(const function<bool()>& atEnd, const function<void()>& parseItem) {
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

    DeclarationPtr parseDeclaration() {
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
    }

    unique_ptr<VariableDeclaration> parseVariableDeclaration() {
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

    unique_ptr<TypeDeclaration> parseTypeDeclaration() {
        Token start = expect(tokenType);
        Token name = expect(tokenIdentifier);
        expect(tokenIs);
        TypePtr type = parseType();
        return make_unique<TypeDeclaration>(start.span, name.ID, std::move(type));
    }

    unique_ptr<RoutineDeclaration> parseRoutineDeclaration() {
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
            bool outerTypedRoutine = insideTypedRoutine;
            insideTypedRoutine = returnType != nullptr;
            body = parseBody();
            insideTypedRoutine = outerTypedRoutine;
            expect(tokenEnd);
        } else if (match(tokenArrow)) {
            expressionBody = parseExpression();
        }
        return make_unique<RoutineDeclaration>(start.span, name.ID, std::move(parameters),
            std::move(returnType), std::move(body), std::move(expressionBody));
    }

    unique_ptr<ParameterDeclaration> parseParameterDeclaration() {
        Token name = expect(tokenIdentifier);
        expect(tokenColon);
        TypePtr type = parseType();
        return make_unique<ParameterDeclaration>(name.span, name.ID, std::move(type));
    }

    TypePtr parseType() {
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
    }

    TypePtr parseArrayType() {
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

    TypePtr parseRecordType() {
        Token start = expect(tokenRecord);
        vector<unique_ptr<VariableDeclaration>> fields;
        parseSequence(
            [this] { return check(tokenEnd); },
            [&] { fields.push_back(parseVariableDeclaration()); });
        expect(tokenEnd);
        return make_unique<RecordType>(start.span, std::move(fields));
    }

    BodyPtr parseBody() {
        Span span = current().span;
        vector<NodePtr> items;
        parseSequence(
            [this] { return check(tokenEnd) || check(tokenElse); },
            [&] { items.push_back(parseBodyItem()); });
        return make_unique<Body>(span, std::move(items));
    }

    NodePtr parseBodyItem() {
        switch (current().code) {
        case tokenVar:
            return parseVariableDeclaration();
        case tokenType:
            return parseTypeDeclaration();
        case tokenIdentifier:
            return parseAssignmentOrCall();
        case tokenWhile:
            return parseWhileLoop();
        case tokenFor:
            return parseForLoop();
        case tokenIf:
            return parseIfStatement();
        case tokenPrint:
            return parsePrintStatement();
        case tokenReturn:
            return parseReturnStatement();
        case tokenRoutine:
            report("Nested routine declarations are not supported");
            return parseRoutineDeclaration();
        default:
            error("declaration or statement");
        }
    }

    StatementPtr parseAssignmentOrCall() {
        Token name = expect(tokenIdentifier);
        if (check(tokenLeftParen)) {
            return make_unique<CallStatement>(name.span, parseRoutineCall(name));
        }
        bool hasAccessors = check(tokenDot) || check(tokenLeftBracket);
        ExpressionPtr target = parseAccessors(make_unique<NameReference>(name.span, name.ID));
        if (match(tokenAssign)) {
            ExpressionPtr value = parseExpression();
            return make_unique<Assignment>(name.span, std::move(target), std::move(value));
        }
        if (!hasAccessors) {
            auto call = make_unique<RoutineCall>(name.span, name.ID, vector<ExpressionPtr>());
            return make_unique<CallStatement>(name.span, std::move(call));
        }
        error("':='");
    }

    StatementPtr parseWhileLoop() {
        Token start = expect(tokenWhile);
        ExpressionPtr condition = parseExpression();
        expect(tokenLoop);
        BodyPtr body = parseBody();
        expect(tokenEnd);
        return make_unique<WhileLoop>(start.span, std::move(condition), std::move(body));
    }

    StatementPtr parseForLoop() {
        Token start = expect(tokenFor);
        Token variable = expect(tokenIdentifier);
        expect(tokenIn);
        ExpressionPtr rangeStart = parseExpression();
        ExpressionPtr rangeEnd;
        if (match(tokenDotDot)) {
            rangeEnd = parseExpression();
        }
        bool reverse = match(tokenReverse);
        expect(tokenLoop);
        BodyPtr body = parseBody();
        expect(tokenEnd);
        return make_unique<ForLoop>(start.span, variable.ID, std::move(rangeStart),
            std::move(rangeEnd), reverse, std::move(body));
    }

    StatementPtr parseIfStatement() {
        Token start = expect(tokenIf);
        ExpressionPtr condition = parseExpression();
        expect(tokenThen);
        BodyPtr thenBody = parseBody();
        BodyPtr elseBody;
        if (match(tokenElse)) {
            elseBody = parseBody();
        }
        expect(tokenEnd);
        return make_unique<IfStatement>(start.span, std::move(condition), std::move(thenBody), std::move(elseBody));
    }

    StatementPtr parsePrintStatement() {
        Token start = expect(tokenPrint);
        vector<ExpressionPtr> values;
        do {
            values.push_back(parseExpression());
        } while (match(tokenComma));
        return make_unique<PrintStatement>(start.span, std::move(values));
    }

    StatementPtr parseReturnStatement() {
        if (!insideTypedRoutine) {
            report("Return statement outside a routine with a result type");
        }
        Token start = expect(tokenReturn);
        ExpressionPtr value = parseExpression();
        return make_unique<ReturnStatement>(start.span, std::move(value));
    }

    ExpressionPtr parseExpression() {
        ExpressionPtr left = parseRelation();
        while (check(tokenAnd) || check(tokenOr) || check(tokenXor)) {
            Token op = advance();
            left = make_unique<BinaryExpression>(op.span, op.code, std::move(left), parseRelation());
        }
        return left;
    }

    ExpressionPtr parseRelation() {
        ExpressionPtr left = parseSimple();
        if (isRelationOperator(current().code)) {
            Token op = advance();
            left = make_unique<BinaryExpression>(op.span, op.code, std::move(left), parseSimple());
        }
        return left;
    }

    ExpressionPtr parseSimple() {
        ExpressionPtr left = parseFactor();
        while (check(tokenPlus) || check(tokenMinus)) {
            Token op = advance();
            left = make_unique<BinaryExpression>(op.span, op.code, std::move(left), parseFactor());
        }
        return left;
    }

    ExpressionPtr parseFactor() {
        ExpressionPtr left = parseSummand();
        while (check(tokenStar) || check(tokenSlash) || check(tokenPercent)) {
            Token op = advance();
            left = make_unique<BinaryExpression>(op.span, op.code, std::move(left), parseSummand());
        }
        return left;
    }

    ExpressionPtr parseSummand() {
        if (match(tokenLeftParen)) {
            ExpressionPtr inner = parseExpression();
            expect(tokenRightParen);
            return inner;
        }
        return parsePrimary();
    }

    ExpressionPtr parsePrimary() {
        switch (current().code) {
        case tokenIntegerLiteral:
        case tokenRealLiteral:
            return parseLiteral();
        case tokenTrue:
        case tokenFalse: {
            Token token = advance();
            return make_unique<BooleanLiteral>(token.span, token.code == tokenTrue);
        }
        case tokenPlus:
        case tokenMinus: {
            Token sign = advance();
            if (!check(tokenIntegerLiteral) && !check(tokenRealLiteral)) {
                error(format("numeric literal after '{}'", tokenName(sign.code)));
            }
            return make_unique<UnaryExpression>(sign.span, sign.code, parseLiteral());
        }
        case tokenNot: {
            Token op = advance();
            if (!check(tokenIntegerLiteral)) {
                error("integer literal after 'not'");
            }
            return make_unique<UnaryExpression>(op.span, op.code, parseLiteral());
        }
        case tokenIdentifier: {
            Token name = advance();
            if (check(tokenLeftParen)) {
                return parseRoutineCall(name);
            }
            return parseAccessors(make_unique<NameReference>(name.span, name.ID));
        }
        default:
            error("expression");
        }
    }

    ExpressionPtr parseLiteral() {
        Token token = advance();
        if (token.code == tokenIntegerLiteral) {
            return make_unique<IntegerLiteral>(token.span, token.intValue);
        }
        return make_unique<RealLiteral>(token.span, token.realValue);
    }

    ExpressionPtr parseAccessors(ExpressionPtr base) {
        while (true) {
            if (check(tokenDot)) {
                Token dot = advance();
                Token member = expect(tokenIdentifier);
                base = make_unique<MemberAccess>(dot.span, std::move(base), member.ID);
            } else if (check(tokenLeftBracket)) {
                Token bracket = advance();
                ExpressionPtr index = parseExpression();
                expect(tokenRightBracket);
                base = make_unique<IndexAccess>(bracket.span, std::move(base), std::move(index));
            } else {
                return base;
            }
        }
    }

    unique_ptr<RoutineCall> parseRoutineCall(const Token& name) {
        expect(tokenLeftParen);
        vector<ExpressionPtr> arguments;
        do {
            arguments.push_back(parseExpression());
        } while (match(tokenComma));
        expect(tokenRightParen);
        return make_unique<RoutineCall>(name.span, name.ID, std::move(arguments));
    }
};

unique_ptr<IParser> makeParser(vector<Token> tokens) {
    return make_unique<Parser>(std::move(tokens));
}
