#ifndef SEMANTIC_CHECKER_RETURN_RETURN_H
#define SEMANTIC_CHECKER_RETURN_RETURN_H

#include "parser/ast.h"
#include "semantic/types/scope_stack.h"

#include <memory>
#include <string>
#include <vector>

using namespace std;

class ICheckerReturn {
protected:
    vector<string>& errors;

public:
    explicit ICheckerReturn(vector<string>& errors) : errors(errors) {}
    virtual ~ICheckerReturn() = default;

    virtual bool checkReturn(
        const ReturnStatement& statement,
        const RoutineDeclaration* routine,
        ScopeStack& scopeStack
    ) = 0;
};

unique_ptr<ICheckerReturn> makeCheckerReturn(vector<string>& errors);

#endif
