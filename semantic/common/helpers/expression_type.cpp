#include "semantic/common/helpers/expression_type.h"

#include <list>

using namespace std;

class ExpressionTypeResolver final : public IExpressionTypeResolver {
private:
    ScopeStack& scopeStack;
    list<TypeInfo> typeCache;

    const TypeInfo* invalid(
        const Node& node,
        const string& message
    );

    const VariableDeclaration* findField(
        const TypeInfo& type,
        const string& member
    ) const;

    const TypeInfo* resolvePrimitiveType(const PrimitiveType& type);
    const TypeInfo* resolveNamedType(const NamedType& type);
    const TypeInfo* resolveArrayType(const ArrayType& type);
    const TypeInfo* resolveRecordType(
        const RecordType& type,
        const TypeDeclaration* declaration
    );
    const TypeInfo* resolveType(
        const TypeNode& type,
        const TypeDeclaration* declaration = nullptr
    ) override;
    const TypeInfo* resolveFieldType(const VariableDeclaration& field);

public:
    ExpressionTypeResolver(
        ScopeStack& scopeStack,
        vector<string>& errors
    );

    const TypeInfo* resolve(const Expression& expression) override;
};

ExpressionTypeResolver::ExpressionTypeResolver(
    ScopeStack& scopeStack,
    vector<string>& errors
) : IExpressionTypeResolver(errors), scopeStack(scopeStack) {}

// ищем поле в описании record
const VariableDeclaration* ExpressionTypeResolver::findField(
    const TypeInfo& type,
    const string& member
) const {
    if (type.kind != TypeKind::Record || type.record == nullptr) {
        return nullptr;
    }

    const RecordType* record = nullptr;
    if (type.record->nodeType == NodeType::TypeDeclaration) {
        const TypeDeclaration* declaration =
            static_cast<const TypeDeclaration*>(type.record);
        record = dynamic_cast<const RecordType*>(declaration->type.get());
    } else if (type.record->nodeType == NodeType::RecordType) {
        record = static_cast<const RecordType*>(type.record);
    }

    if (record == nullptr) {
        return nullptr;
    }

    for (const auto& field : record->fields) {
        if (field->name == member) {
            return field.get();
        }
    }

    return nullptr;
}

const TypeInfo* ExpressionTypeResolver::resolvePrimitiveType(
    const PrimitiveType& type
) {
    switch (type.kind) {
    case tokenInteger:
        return &integerType;
    case tokenReal:
        return &realType;
    case tokenBoolean:
        return &booleanType;
    default:
        return &errorType;
    }
}

const TypeInfo* ExpressionTypeResolver::resolveNamedType(
    const NamedType& type
) {
    Symbol* symbol = scopeStack.lookup(type.name);
    if (symbol == nullptr) {
        return invalid(type, "undeclared type '" + type.name + "'");
    }

    if (symbol->kind != SymbolKind::Type || symbol->type == nullptr) {
        return invalid(type, "'" + type.name + "' is not a type");
    }

    return symbol->type;
}

const TypeInfo* ExpressionTypeResolver::resolveArrayType(
    const ArrayType& type
) {
    if (type.elementType == nullptr) {
        return &errorType;
    }

    const TypeInfo* elementType = resolveType(*type.elementType);
    if (elementType->kind == TypeKind::Error) {
        return &errorType;
    }

    return &typeCache.emplace_back(arrayType(*elementType));
}

const TypeInfo* ExpressionTypeResolver::resolveRecordType(
    const RecordType& type,
    const TypeDeclaration* declaration
) {
    if (declaration != nullptr) {
        return &typeCache.emplace_back(recordType(*declaration));
    }

    return &typeCache.emplace_back(recordType(type));
}

const TypeInfo* ExpressionTypeResolver::resolveType(
    const TypeNode& type,
    const TypeDeclaration* declaration
) {
    switch (type.nodeType) {
    case NodeType::PrimitiveType:
        return resolvePrimitiveType(static_cast<const PrimitiveType&>(type));
    case NodeType::NamedType:
        return resolveNamedType(static_cast<const NamedType&>(type));
    case NodeType::ArrayType:
        return resolveArrayType(static_cast<const ArrayType&>(type));
    case NodeType::RecordType:
        return resolveRecordType(
            static_cast<const RecordType&>(type),
            declaration
        );
    default:
        return &errorType;
    }
}

// получаем тип конкретного поля
const TypeInfo* ExpressionTypeResolver::resolveFieldType(
    const VariableDeclaration& field
) {
    if (field.type == nullptr) {
        return &errorType;
    }

    return resolveType(*field.type);
}

const TypeInfo* ExpressionTypeResolver::invalid(
    const Node& node,
    const string& message
) {
    errors.push_back(
        "Semantic error (line " + to_string(node.span.lineNum) +
        "): " + message
    );
    return &errorType;
}

// определяем тип переданного выражения
unique_ptr<IExpressionTypeResolver> makeExpressionTypeResolver(
    ScopeStack& scopeStack,
    vector<string>& errors
) {
    return make_unique<ExpressionTypeResolver>(scopeStack, errors);
}

const TypeInfo* ExpressionTypeResolver::resolve(const Expression& expression) {
    switch (expression.nodeType) {
    // у литералов тип известен сразу
    case NodeType::IntegerLiteral:
        return &integerType;
    case NodeType::RealLiteral:
        return &realType;
    case NodeType::BooleanLiteral:
        return &booleanType;

    // ищем переменную в скоупах и возвращаем ее тип
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

    // проверяем тип операнда унарного выражения
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

    // проверяем типы левого и правого операнда бинарного выражения
    case NodeType::BinaryExpression: {
        const BinaryExpression& binary =
            static_cast<const BinaryExpression&>(expression);
        const TypeInfo* left = resolve(*binary.left);
        const TypeInfo* right = resolve(*binary.right);

        if (left->kind == TypeKind::Error ||
            right->kind == TypeKind::Error) {
            return &errorType;
        }

        // проверяем что оба типа числовые
        bool numeric =
            (left->kind == TypeKind::Integer || left->kind == TypeKind::Real) &&
            (right->kind == TypeKind::Integer || right->kind == TypeKind::Real);

        switch (binary.op) {
        // арифметические операции работают только с числами
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

        // остаток от деления работает только с integer
        case tokenPercent:
            if (left->kind != TypeKind::Integer ||
                right->kind != TypeKind::Integer) {
                return invalid(
                    expression,
                    "remainder operator '%' requires integer operands"
                );
            }
            return &integerType;

        // логические операции работают только с boolean
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

        // операции сравнения возвращают boolean
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

        // равенство можно проверять у одинаковых или числовых типов
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

    // получаем тип результата вызова routine
    case NodeType::RoutineCall: {
        const RoutineCall& call =
            static_cast<const RoutineCall&>(expression);
        Symbol* symbol = scopeStack.lookup(call.name);

        if (symbol == nullptr || symbol->kind != SymbolKind::Routine) {
            return invalid(expression, "'" + call.name + "' is not a routine");
        }
        if (symbol->type == nullptr) {
            return &errorType;
        }
        return symbol->type;
    }

    // проверяем индексирование массива и возвращаем тип его элемента
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

    // проверяем доступ к полю record и возвращаем тип поля
    case NodeType::MemberAccess: {
        const MemberAccess& access =
            static_cast<const MemberAccess&>(expression);
        // сначала получаем тип объекта, у которого запрашиваем поле
        const TypeInfo* objectType = resolve(*access.object);

        if (objectType->kind == TypeKind::Error) {
            return &errorType;
        }

        if (objectType->kind != TypeKind::Record ||
            objectType->record == nullptr) {
            return invalid(
                expression,
                "member access requires a record expression"
            );
        }

        // ищем поле с таким именем в record
        const VariableDeclaration* field =
            findField(*objectType, access.member);
        if (field == nullptr) {
            return invalid(
                expression,
                "record has no field '" + access.member + "'"
            );
        }

        // получаем тип найденного поля
        const TypeInfo* fieldType = resolveFieldType(*field);
        if (fieldType->kind == TypeKind::Error) {
            return invalid(
                expression,
                "cannot determine the type of field '" + access.member + "'"
            );
        }

        return fieldType;
    }

    default:
        return invalid(expression, "cannot determine the type of this expression");
    }
}
