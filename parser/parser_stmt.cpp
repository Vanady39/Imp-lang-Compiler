#include "parser/parser_impl.h"

using namespace std;

// Операторы.

BodyPtr Parser::parseBody() {
    Span span = current().span;
    vector<NodePtr> items;
    parseSequence(
        [this] { return check(tokenEnd) || check(tokenElse); },
        [&] { items.push_back(parseBodyItem()); });
    return make_unique<Body>(span, std::move(items));
}

NodePtr Parser::parseBodyItem() {
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
    return nullptr;
}

StatementPtr Parser::parseAssignmentOrCall() {
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
    return nullptr;
}

StatementPtr Parser::parseWhileLoop() {
    Token start = expect(tokenWhile);
    ExpressionPtr condition = parseExpression();
    expect(tokenLoop);
    BodyPtr body = parseBody();
    expect(tokenEnd);
    return make_unique<WhileLoop>(start.span, std::move(condition), std::move(body));
}

StatementPtr Parser::parseForLoop() {
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

StatementPtr Parser::parseIfStatement() {
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

StatementPtr Parser::parsePrintStatement() {
    Token start = expect(tokenPrint);
    vector<ExpressionPtr> values;
    do {
        values.push_back(parseExpression());
    } while (match(tokenComma));
    return make_unique<PrintStatement>(start.span, std::move(values));
}

StatementPtr Parser::parseReturnStatement() {
    Token start = expect(tokenReturn);
    ExpressionPtr value = parseExpression();
    return make_unique<ReturnStatement>(start.span, std::move(value));
}
