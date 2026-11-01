#ifndef SEMANTIC_COMMON_HELPERS_EXPRESSION_TYPE_H
#define SEMANTIC_COMMON_HELPERS_EXPRESSION_TYPE_H

#include "semantic/types/scope_stack.h"

#include <string>
#include <vector>

using namespace std;

class ExpressionTypeResolver {
private:
    ScopeStack& scopeStack;
    vector<string>& errors;

    const TypeInfo* invalid(
        const Expression& expression,
        const string& message
    );

public:
    ExpressionTypeResolver(
        ScopeStack& scopeStack,
        vector<string>& errors
    );

    const TypeInfo* resolve(const Expression& expression);
};

#endif
