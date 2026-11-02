#ifndef SEMANTIC_CHECKER_ROUTINE_ROUTINE_H
#define SEMANTIC_CHECKER_ROUTINE_ROUTINE_H

#include "parser/ast.h"
#include "semantic/types/scope_stack.h"

#include <memory>
#include <string>
#include <vector>

using namespace std;

class ICheckerRoutine {
protected:
    vector<string>& errors;

public:
    explicit ICheckerRoutine(vector<string>& errors) : errors(errors) {}
    virtual ~ICheckerRoutine() = default;

    virtual bool declareRoutine(
        const RoutineDeclaration& declaration,
        const vector<const TypeInfo*>& parameterTypes,
        const TypeInfo* returnType,
        ScopeStack& scopeStack
    ) = 0;
};

unique_ptr<ICheckerRoutine> makeCheckerRoutine(vector<string>& errors);

#endif