#include "semantic/optimizer/expression_optimizer/expression_optimizer.h"

#include "lexer/lexer.h"

using namespace std;

namespace optimizer_detail {
namespace {

ExpressionPtr foldUnary(ExpressionPtr expression) {
    auto* unary = static_cast<UnaryExpression*>(expression.get());
    if (unary->operand == nullptr) {
        return expression;
    }

    if (unary->op == tokenMinus) {
        if (unary->operand->nodeType == NodeType::IntegerLiteral) {
            auto* value = static_cast<IntegerLiteral*>(unary->operand.get());
            return make_unique<IntegerLiteral>(unary->span, -value->value);
        }
        if (unary->operand->nodeType == NodeType::RealLiteral) {
            auto* value = static_cast<RealLiteral*>(unary->operand.get());
            return make_unique<RealLiteral>(unary->span, -value->value);
        }
    }

    if (unary->op == tokenPlus) {
        if (unary->operand->nodeType == NodeType::IntegerLiteral) {
            auto* value = static_cast<IntegerLiteral*>(unary->operand.get());
            return make_unique<IntegerLiteral>(unary->span, value->value);
        }
        if (unary->operand->nodeType == NodeType::RealLiteral) {
            auto* value = static_cast<RealLiteral*>(unary->operand.get());
            return make_unique<RealLiteral>(unary->span, value->value);
        }
    }

    return expression;
}

// Сворачиваются только числовые литералы. Операции с именами и
// вызовами остаются в AST, потому что их значение неизвестно заранее.
ExpressionPtr foldBinary(ExpressionPtr expression) {
    auto* binary = static_cast<BinaryExpression*>(expression.get());
    if (binary->left == nullptr || binary->right == nullptr) {
        return expression;
    }

    bool leftInteger = binary->left->nodeType == NodeType::IntegerLiteral;
    bool leftReal = binary->left->nodeType == NodeType::RealLiteral;
    bool rightInteger = binary->right->nodeType == NodeType::IntegerLiteral;
    bool rightReal = binary->right->nodeType == NodeType::RealLiteral;
    if ((!leftInteger && !leftReal) || (!rightInteger && !rightReal)) {
        return expression;
    }

    double left = leftInteger
        ? static_cast<IntegerLiteral*>(binary->left.get())->value
        : static_cast<RealLiteral*>(binary->left.get())->value;
    double right = rightInteger
        ? static_cast<IntegerLiteral*>(binary->right.get())->value
        : static_cast<RealLiteral*>(binary->right.get())->value;

    // Деление и остаток с нулевым знаменателем оставляем для семантики
    // языка, чтобы оптимизатор не менял момент возникновения ошибки.
    if ((binary->op == tokenSlash || binary->op == tokenPercent) &&
        right == 0.0) {
        return expression;
    }

    if (binary->op == tokenPercent && (!leftInteger || !rightInteger)) {
        return expression;
    }

    double value = 0.0;
    switch (binary->op) {
    case tokenPlus:
        value = left + right;
        break;
    case tokenMinus:
        value = left - right;
        break;
    case tokenStar:
        value = left * right;
        break;
    case tokenSlash:
        value = left / right;
        break;
    case tokenPercent:
        value = static_cast<int>(left) % static_cast<int>(right);
        break;
    default:
        return expression;
    }

    if (leftInteger && rightInteger && binary->op != tokenSlash) {
        return make_unique<IntegerLiteral>(binary->span, static_cast<int>(value));
    }

    return make_unique<RealLiteral>(binary->span, value);
}

} // namespace

ExpressionPtr optimizeExpression(ExpressionPtr expression) {
    if (expression == nullptr) {
        return nullptr;
    }

    switch (expression->nodeType) {
    case NodeType::UnaryExpression: {
        auto* unary = static_cast<UnaryExpression*>(expression.get());
        unary->operand = optimizeExpression(std::move(unary->operand));
        return foldUnary(std::move(expression));
    }
    case NodeType::BinaryExpression: {
        auto* binary = static_cast<BinaryExpression*>(expression.get());
        binary->left = optimizeExpression(std::move(binary->left));
        binary->right = optimizeExpression(std::move(binary->right));
        return foldBinary(std::move(expression));
    }
    case NodeType::MemberAccess: {
        auto* member = static_cast<MemberAccess*>(expression.get());
        member->object = optimizeExpression(std::move(member->object));
        break;
    }
    case NodeType::IndexAccess: {
        auto* index = static_cast<IndexAccess*>(expression.get());
        index->array = optimizeExpression(std::move(index->array));
        index->index = optimizeExpression(std::move(index->index));
        break;
    }
    case NodeType::RoutineCall: {
        auto* call = static_cast<RoutineCall*>(expression.get());
        for (auto& argument : call->arguments) {
            argument = optimizeExpression(std::move(argument));
        }
        break;
    }
    case NodeType::IntegerLiteral:
    case NodeType::RealLiteral:
    case NodeType::BooleanLiteral:
    case NodeType::NameReference:
        break;
    default:
        break;
    }

    return expression;
}

} // namespace optimizer_detail
