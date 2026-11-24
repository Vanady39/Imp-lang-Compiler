#include "semantic/checker/return/return.h"

#include "semantic/common/helpers/expression_type.h"

using namespace std;

class CheckerReturn final : public ICheckerReturn {
public:
    explicit CheckerReturn(vector<string>& errors) : ICheckerReturn(errors) {}

    bool checkReturn(
        const Expression& value,
        const RoutineDeclaration* routine,
        const TypeInfo* expected,
        ScopeStack& scopeStack
    ) override {
        const string location = "Semantic error (line " + to_string(value.span.lineNum) + "): ";
        if (routine == nullptr) {
            errors.push_back(location + "return is only allowed inside a routine");
            return false;
        }
        if (expected == nullptr) {
            errors.push_back(location + "routine '" + routine->name + "' does not return a value");
            return false;
        }

        auto resolver = makeExpressionTypeResolver(scopeStack, errors);
        const TypeInfo* actual = resolver->resolve(value);
        if (actual == nullptr || expected->kind == TypeKind::Error ||
            actual->kind == TypeKind::Error) {
            return false;
        }
        if (!isAssignmentCompatible(*expected, *actual)) {
            errors.push_back(location + "return value has incompatible type");
            return false;
        }
        return true;
    }
};

unique_ptr<ICheckerReturn> makeCheckerReturn(vector<string>& errors) {
    return make_unique<CheckerReturn>(errors);
}
