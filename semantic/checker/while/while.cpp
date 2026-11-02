#include "semantic/checker/while/while.h"
#include "semantic/common/helpers/expression_type.h"
#include "semantic/types/scope_stack.h"

using namespace std;

class CheckerWhile final : public ICheckerWhile {
private:
    // Резолвер использует область видимости, подготовленную вызывающим кодом.
    ScopeStack& scopes;

public:
    CheckerWhile(
        ScopeStack& scopesStack,
        vector<string>& errors
    ) : ICheckerWhile(errors),
        scopes(scopesStack) {}

    bool checkCondition(
        const WhileLoop& whileLoop
    ) override {
        if (whileLoop.condition == nullptr) {
            errors.push_back(
                "Semantic error (line " +
                to_string(whileLoop.span.lineNum) +
                "): while condition is missing"
            );
            return false;
        }

        // unique_ptr владеет выражением, а checker только временно обращается к нему.
        const Expression& expression = *whileLoop.condition;

        // Определение типа передаём существующему резолверу выражений.
        unique_ptr<IExpressionTypeResolver> typeResolver =
            makeExpressionTypeResolver(scopes, errors);
        const TypeInfo* resolvedType = typeResolver->resolve(expression);

        if (resolvedType == nullptr) {
            errors.push_back(
                "Semantic error (line " +
                to_string(whileLoop.span.lineNum) +
                "): cannot determine type of while condition"
            );
            return false;
        }

        // Резолвер уже сообщил об ошибке, поэтому не добавляем вторую.
        if (resolvedType->kind == TypeKind::Error) {
            return false;
        }

        if (resolvedType->kind != TypeKind::Boolean) {
            errors.push_back(
                "Semantic error (line " +
                to_string(whileLoop.span.lineNum) +
                "): while condition must have type 'boolean'"
            );
            return false;
        }

        return true;
    }
};

unique_ptr<ICheckerWhile> makeCheckerWhile(
    ScopeStack& scopes,
    vector<string>& errors
) {
    return make_unique<CheckerWhile>(scopes, errors);
}
