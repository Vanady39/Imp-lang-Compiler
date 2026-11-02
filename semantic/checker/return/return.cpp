#include "semantic/checker/return/return.h"

#include "semantic/common/helpers/expression_type.h"

using namespace std;

class CheckerReturn final : public ICheckerReturn {
private:
    bool areCompatible(
        const TypeInfo& targetType,
        const TypeInfo& valueType
    ) const {
        if (targetType.kind == TypeKind::Error ||
            valueType.kind == TypeKind::Error) {
            return true;
        }

        return sameType(targetType, valueType) ||
            (targetType.kind == TypeKind::Boolean &&
             valueType.kind == TypeKind::Integer) ||
            (targetType.kind == TypeKind::Integer &&
             valueType.kind == TypeKind::Boolean) ||
            (targetType.kind == TypeKind::Integer &&
             valueType.kind == TypeKind::Real) ||
            (targetType.kind == TypeKind::Real &&
             valueType.kind == TypeKind::Integer);
    }

public:
    explicit CheckerReturn(vector<string>& errors)
        : ICheckerReturn(errors) {}

    bool checkReturn(
        const ReturnStatement& statement,
        const RoutineDeclaration* routine,
        ScopeStack& scopeStack
    ) override {
        if (routine == nullptr) {
            errors.push_back(
                "Semantic error (line " +
                to_string(statement.span.lineNum) +
                "): return is only allowed inside a routine"
            );
            return false;
        }

        if (routine->returnType == nullptr) {
            errors.push_back(
                "Semantic error (line " +
                to_string(statement.span.lineNum) +
                "): routine '" + routine->name +
                "' does not return a value"
            );
            return false;
        }

        // Парсер сейчас всегда создаёт expression после ключевого слова
        // return, но проверка сохраняет это правило и для будущих AST.
        if (statement.value == nullptr) {
            errors.push_back(
                "Semantic error (line " +
                to_string(statement.span.lineNum) +
                "): return value is required in routine '" +
                routine->name + "'"
            );
            return false;
        }

        unique_ptr<IExpressionTypeResolver> resolver =
            makeExpressionTypeResolver(scopeStack, errors);
        const TypeInfo* expected = resolver->resolveType(*routine->returnType);
        const TypeInfo* actual = resolver->resolve(*statement.value);

        if (expected == nullptr || actual == nullptr ||
            expected->kind == TypeKind::Error ||
            actual->kind == TypeKind::Error) {
            return false;
        }

        if (!areCompatible(*expected, *actual)) {
            errors.push_back(
                "Semantic error (line " +
                to_string(statement.span.lineNum) +
                "): return value has incompatible type"
            );
            return false;
        }

        return true;
    }
};

unique_ptr<ICheckerReturn> makeCheckerReturn(vector<string>& errors) {
    return make_unique<CheckerReturn>(errors);
}
