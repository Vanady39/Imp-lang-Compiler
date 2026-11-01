#ifndef SEMANTIC_CHECKER_VARIABLE_VARIABLE_H
#define SEMANTIC_CHECKER_VARIABLE_VARIABLE_H

#include "parser/ast.h"
#include "shared/types/type_info.h"
#include "semantic/types/scope_stack.h"

#include <memory>
#include <string>
#include <vector>

using namespace std;

class ICheckerVariable {
protected:
    vector<string>& errors;

public:
    explicit ICheckerVariable(vector<string>& errors) : errors(errors) {}
    virtual ~ICheckerVariable() = default;

    virtual bool declareVariable(
        const VariableDeclaration& declaration,
        const TypeInfo* type,
        ScopeStack& scopeStack
    ) = 0;

    virtual TypeInfo checkNameReference(
        const NameReference& expression,
        ScopeStack& scopeStack
    ) = 0;

    virtual bool checkAssignmentTarget(
        const NameReference& expression,
        ScopeStack& scopeStack
    ) = 0;
};

unique_ptr<ICheckerVariable> makeCheckerVariable(vector<string>& errors);

#endif
