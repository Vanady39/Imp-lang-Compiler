#include "parser/parser_impl.h"

#include <format>

using namespace std;

namespace {

// Приоритеты бинарных операторов: чем больше число, тем сильнее оператор
// связывает. Уровни левоассоциативные. Сравнения не образуют цепочек:
// по A6 (DECISIONS.md) Relation допускает не более одного сравнения.
const int relationPrecedence = 3;

struct BinaryOperator {
    TokenCode code;
    int precedence;
};

const BinaryOperator binaryOperators[] = {
    { tokenOr, 1 }, { tokenXor, 1 },
    { tokenAnd, 2 },
    { tokenEqual, 3 }, { tokenNotEqual, 3 },
    { tokenLess, 3 }, { tokenLessEqual, 3 },
    { tokenGreater, 3 }, { tokenGreaterEqual, 3 },
    { tokenPlus, 4 }, { tokenMinus, 4 },
    { tokenStar, 5 }, { tokenSlash, 5 }, { tokenPercent, 5 },
};

const BinaryOperator* findBinaryOperator(TokenCode code) {
    for (const BinaryOperator& op : binaryOperators) {
        if (op.code == code) {
            return &op;
        }
    }
    return nullptr;
}

} // namespace

// Выражения: precedence climbing. parseBinary разбирает операторы уровня
// minPrecedence и выше; каждый parsePrimary даёт очередной операнд.

ExpressionPtr Parser::parseExpression() {
    return parseBinary(1);
}

ExpressionPtr Parser::parseBinary(int minPrecedence) {
    ExpressionPtr left = parsePrimary();
    bool comparisonSeen = false;
    while (true) {
        const BinaryOperator* op = findBinaryOperator(current().code);
        if (!op || op->precedence < minPrecedence
            || (comparisonSeen && op->precedence == relationPrecedence)) {
            return left;
        }
        Token opToken = advance();
        comparisonSeen = op->precedence == relationPrecedence;
        left = make_unique<BinaryExpression>(opToken.span, opToken.code, std::move(left),
            parseBinary(op->precedence + 1));
    }
}

ExpressionPtr Parser::parsePrimary() {
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
    case tokenLeftParen: {
        advance();
        ExpressionPtr inner = parseExpression();
        expect(tokenRightParen);
        return inner;
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
    return nullptr;
}

ExpressionPtr Parser::parseLiteral() {
    Token token = advance();
    if (token.code == tokenIntegerLiteral) {
        return make_unique<IntegerLiteral>(token.span, token.intValue);
    }
    return make_unique<RealLiteral>(token.span, token.realValue);
}

ExpressionPtr Parser::parseAccessors(ExpressionPtr base) {
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

unique_ptr<RoutineCall> Parser::parseRoutineCall(const Token& name) {
    expect(tokenLeftParen);
    vector<ExpressionPtr> arguments;
    do {
        arguments.push_back(parseExpression());
    } while (match(tokenComma));
    expect(tokenRightParen);
    return make_unique<RoutineCall>(name.span, name.ID, std::move(arguments));
}
