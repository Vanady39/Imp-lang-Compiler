#include "checker.h"
#include "semantic/checker/variable/variable.h"
#include "semantic/types/scope_stack.h"

using namespace std;

class Checker : public IChecker {
private:
    vector<string> errors;
    ScopeStack scopes;
    CheckerVariable variableChecker;

public:
    Checker() : variableChecker(errors) {}

    CheckResult check(Program& program) override {
        (void)program;
        return {};
    }

    const vector<string>& get_errors() const override {
        return errors;
    }

    bool declareVariable(
        const VariableDeclaration& declaration,
        const TypeInfo* type,
        ScopeStack& ScopeStack
    ) {
        return variableChecker.declareVariable(declaration, type, ScopeStack);
    }

    TypeInfo checkNameReference(
        const NameReference& expression,
        ScopeStack& scopeStack
    ) {
        return variableChecker.checkNameReference(expression, scopeStack);
    }
};
