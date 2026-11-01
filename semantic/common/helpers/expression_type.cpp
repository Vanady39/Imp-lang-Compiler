#include "semantic/common/helpers/expression_type.h"

using namespace std;

ExpressionTypeResolver::ExpressionTypeResolver(
    ScopeStack& scopeStack,
    vector<string>& errors
) : scopeStack(scopeStack), errors(errors) {}

const TypeInfo* ExpressionTypeResolver::invalid(
    const Expression& expression,
    const string& message
) {
    errors.push_back(
        "Semantic error (line " + to_string(expression.span.lineNum) +
        "): " + message
    );
    return &errorType;
}

const TypeInfo* ExpressionTypeResolver::resolve(const Expression& expression) {
    switch (expression.nodeType) {
    case NodeType::IntegerLiteral:
        return &integerType;
    case NodeType::RealLiteral:
        return &realType;
    case NodeType::BooleanLiteral:
        return &booleanType;

    case NodeType::NameReference: {
        const NameReference& reference =
            static_cast<const NameReference&>(expression);
        Symbol* symbol = scopeStack.lookup(reference.name);

        if (symbol == nullptr) {
            return invalid(expression, "undeclared name '" + reference.name + "'");
        }
        if (symbol->kind != SymbolKind::Variable &&
            symbol->kind != SymbolKind::Parameter) {
            return invalid(
                expression,
                "'" + reference.name + "' is not a variable or parameter"
            );
        }
        if (symbol->type == nullptr) {
            return invalid(
                expression,
                "cannot determine the type of '" + reference.name + "'"
            );
        }
        return symbol->type;
    }

    case NodeType::UnaryExpression: {
        const UnaryExpression& unary =
            static_cast<const UnaryExpression&>(expression);
        const TypeInfo* operand = resolve(*unary.operand);

        if (operand->kind == TypeKind::Error) {
            return &errorType;
        }
        if ((unary.op == tokenPlus || unary.op == tokenMinus) &&
            (operand->kind == TypeKind::Integer ||
             operand->kind == TypeKind::Real)) {
            return operand;
        }
        if (unary.op == tokenNot && operand->kind == TypeKind::Boolean) {
            return &booleanType;
        }
        return invalid(
            expression,
            "invalid operand type for unary operator '" +
            tokenName(unary.op) + "'"
        );
    }

    case NodeType::BinaryExpression: {
        const BinaryExpression& binary =
            static_cast<const BinaryExpression&>(expression);
        const TypeInfo* left = resolve(*binary.left);
        const TypeInfo* right = resolve(*binary.right);

        if (left->kind == TypeKind::Error ||
            right->kind == TypeKind::Error) {
            return &errorType;
        }

        bool numeric =
            (left->kind == TypeKind::Integer || left->kind == TypeKind::Real) &&
            (right->kind == TypeKind::Integer || right->kind == TypeKind::Real);

        switch (binary.op) {
        case tokenPlus:
        case tokenMinus:
        case tokenStar:
        case tokenSlash:
            if (!numeric) {
                return invalid(
                    expression,
                    "arithmetic operator '" + tokenName(binary.op) +
                    "' requires numeric operands"
                );
            }
            if (left->kind == TypeKind::Real ||
                right->kind == TypeKind::Real) {
                return &realType;
            }
            return &integerType;

        case tokenPercent:
            if (left->kind != TypeKind::Integer ||
                right->kind != TypeKind::Integer) {
                return invalid(
                    expression,
                    "remainder operator '%' requires integer operands"
                );
            }
            return &integerType;

        case tokenAnd:
        case tokenOr:
        case tokenXor:
            if (left->kind != TypeKind::Boolean ||
                right->kind != TypeKind::Boolean) {
                return invalid(
                    expression,
                    "logical operator '" + tokenName(binary.op) +
                    "' requires boolean operands"
                );
            }
            return &booleanType;

        case tokenLess:
        case tokenLessEqual:
        case tokenGreater:
        case tokenGreaterEqual:
            if (!numeric) {
                return invalid(
                    expression,
                    "comparison operator '" + tokenName(binary.op) +
                    "' requires numeric operands"
                );
            }
            return &booleanType;

        case tokenEqual:
        case tokenNotEqual:
            if (!sameType(*left, *right) && !numeric) {
                return invalid(
                    expression,
                    "operands of comparison '" + tokenName(binary.op) +
                    "' have incompatible types"
                );
            }
            return &booleanType;

        default:
            return invalid(
                expression,
                "unsupported binary operator '" + tokenName(binary.op) + "'"
            );
        }
    }

    case NodeType::RoutineCall: {
        const RoutineCall& call =
            static_cast<const RoutineCall&>(expression);
        Symbol* symbol = scopeStack.lookup(call.name);

        if (symbol == nullptr || symbol->kind != SymbolKind::Routine) {
            return invalid(expression, "'" + call.name + "' is not a routine");
        }
        if (symbol->type == nullptr) {
            return invalid(
                expression,
                "routine '" + call.name + "' has no result type"
            );
        }
        return symbol->type;
    }

    case NodeType::IndexAccess: {
        const IndexAccess& access =
            static_cast<const IndexAccess&>(expression);
        const TypeInfo* array = resolve(*access.array);
        const TypeInfo* index = resolve(*access.index);

        if (array->kind == TypeKind::Error ||
            index->kind == TypeKind::Error) {
            return &errorType;
        }
        if (array->kind != TypeKind::Array || array->element == nullptr) {
            return invalid(expression, "indexed expression is not an array");
        }
        if (index->kind != TypeKind::Integer) {
            return invalid(expression, "array index must have type 'integer'");
        }
        return array->element;
    }

    case NodeType::MemberAccess:
        return invalid(
            expression,
            "record member types are not available to the type resolver"
        );

    default:
        return invalid(expression, "cannot determine the type of this expression");
    }
}
