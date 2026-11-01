#ifndef SEMANTIC_CHECKER_VARIABLE_VARIABLE_H
#define SEMANTIC_CHECKER_VARIABLE_VARIABLE_H

#include "parser/ast.h"
#include "shared/types/type_info.h"
#include "semantic/types/scope_stack.h"

#include <string>
#include <vector>

using namespace std;

class ICheckerVariable {
protected:
    vector<string>& errors;

public:
    ICheckerVariable(vector<string>& errors) : errors(errors) {}
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
};

class CheckerVariable : public ICheckerVariable {
public:
    explicit CheckerVariable(vector<string>& errors);

    bool declareVariable(
        const VariableDeclaration& declaration,
        const TypeInfo* type,
        ScopeStack& scopeStack
    ) override;

    TypeInfo checkNameReference(
        const NameReference& expression,
        ScopeStack& scopeStack
    ) override;
};

#endif
