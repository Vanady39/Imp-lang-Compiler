#ifndef SEMANTIC_CHECKER_FOR_LOOP_H
#define SEMANTIC_CHECKER_FOR_LOOP_H

#include "parser/ast.h"
#include "semantic/types/scope_stack.h"

#include <memory>
#include <string>
#include <vector>

using namespace std;

class ICheckerFor {
protected: 
    vector<string>& errors;

public:
    explicit ICheckerFor(vector<string>& errors) : errors(errors) {}
    virtual ~ICheckerFor() = default;

    virtual bool checkNumberRange(
        const ForLoop& forLoop,
        ScopeStack& scopeStack
    ) = 0;

    virtual bool checkArrayRange(
        const ForLoop& forLoop,
        ScopeStack& scopeStack
    ) = 0;
};

unique_ptr<ICheckerFor> makeCheckerFor(vector<string>& errors);

#endif