#include "semantic/checker/condition/condition.h"
#include "semantic/common/helpers/expression_type.h"

using namespace std;

class CheckerCondition final : public ICheckerCondition {
private:
    // Резолвер использует область видимости, подготовленную вызывающим кодом.
    ScopeStack& scopes;

public:
    CheckerCondition(
        ScopeStack& scopesStack,
        vector<string>& errors
    ) : ICheckerCondition(errors), scopes(scopesStack) {}

    bool checkCondition(
        const IfStatement& ifStatement
    ) override {
        if (ifStatement.condition == nullptr) {
            errors.push_back(
                "Semantic error (line " +
                to_string(ifStatement.span.lineNum) +
                "): if condition is missing"
            );
            return false;
        }

        // unique_ptr владеет выражением, а checker только временно обращается к нему.
        const Expression& expression = *ifStatement.condition;

        // Определение типа передаём существующему резолверу выражений.
        unique_ptr<IExpressionTypeResolver> typeResolver =
            makeExpressionTypeResolver(scopes, errors);
        const TypeInfo* resolvedType = typeResolver->resolve(expression);

        if (resolvedType == nullptr) {
            errors.push_back(
                "Semantic error (line " +
                to_string(ifStatement.span.lineNum) +
                "): cannot determine type of if condition"
            );
            return false;
        }

        if (resolvedType->kind == TypeKind::Error) {
            return false;
        }

        if (resolvedType->kind != TypeKind::Boolean) {
            errors.push_back(
                "Semantic error (line " +
                to_string(ifStatement.span.lineNum) +
                "): if condition must have type 'boolean'"
            );
            return false;
        }

        return true;
    }
};

unique_ptr<ICheckerCondition> makeCheckerCondition(
    ScopeStack& scopes,
    vector<string>& errors
) {
    return make_unique<CheckerCondition>(scopes, errors);
}