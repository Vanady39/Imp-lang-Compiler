#ifndef SHARED_TYPES_CONSTANT_INTEGER_H
#define SHARED_TYPES_CONSTANT_INTEGER_H

#include "parser/ast.h"

#include <cstdint>
#include <limits>
#include <optional>

inline std::optional<int> checkedInteger(std::int64_t value) {
    if (value < std::numeric_limits<int>::min() || value > std::numeric_limits<int>::max()) {
        return std::nullopt;
    }
    return static_cast<int>(value);
}

inline std::optional<int> constantIntegerBinary(TokenCode op, int left, int right) {
    const std::int64_t lhs = left;
    const std::int64_t rhs = right;
    switch (op) {
    case tokenPlus: return checkedInteger(lhs + rhs);
    case tokenMinus: return checkedInteger(lhs - rhs);
    case tokenStar: return checkedInteger(lhs * rhs);
    case tokenSlash: return rhs == 0 ? std::nullopt : checkedInteger(lhs / rhs);
    case tokenPercent: return rhs == 0 ? std::nullopt : checkedInteger(lhs % rhs);
    default: return std::nullopt;
    }
}

inline std::optional<int> constantInteger(const Expression& expr) {
    switch (expr.nodeType) {
    case NodeType::IntegerLiteral:
        return static_cast<const IntegerLiteral&>(expr).value;
    case NodeType::UnaryExpression: {
        const auto& unary = static_cast<const UnaryExpression&>(expr);
        const auto operand = constantInteger(*unary.operand);
        if (!operand) return std::nullopt;
        if (unary.op == tokenPlus) return operand;
        if (unary.op == tokenMinus) return checkedInteger(-static_cast<std::int64_t>(*operand));
        return std::nullopt;
    }
    case NodeType::BinaryExpression: {
        const auto& binary = static_cast<const BinaryExpression&>(expr);
        const auto left = constantInteger(*binary.left);
        const auto right = constantInteger(*binary.right);
        if (!left || !right) return std::nullopt;
        return constantIntegerBinary(binary.op, *left, *right);
    }
    default:
        return std::nullopt;
    }
}

#endif
