#ifndef SEMANTIC_CHECKER_CONDITION
#define SEMANTIC_CHECKER_CONDITION

#include "parser/ast.h"
#include "semantic/types/scope_stack.h"

#include <memory>
#include <string>
#include <vector>

using namespace std;

class ICheckerCondition {
protected:
    vector<string>& errors;

public:
    explicit ICheckerCondition(vector<string>& errors)
        : errors(errors) {}

    virtual ~ICheckerCondition() = default;

    virtual bool checkCondition(
        const IfStatement& ifStatement
    ) = 0;
};

unique_ptr<ICheckerCondition> makeCheckerCondition(
    ScopeStack& scopes,
    vector<string>& errors
);

#endif