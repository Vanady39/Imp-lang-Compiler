#ifndef SEMANTIC_COMMON_HELPERS_EXPRESSION_TYPE_H
#define SEMANTIC_COMMON_HELPERS_EXPRESSION_TYPE_H

#include "semantic/types/scope_stack.h"

#include <list>
#include <string>
#include <vector>

using namespace std;

class ExpressionTypeResolver {
private:
    ScopeStack& scopeStack;
    vector<string>& errors;
    list<TypeInfo> typeCache;

    const TypeInfo* invalid(
        const Expression& expression,
        const string& message
    );

    const VariableDeclaration* findField(
        const TypeInfo& type,
        const string& member
    ) const;

    const TypeInfo* resolveType(const TypeNode& type);
    const TypeInfo* resolveFieldType(const VariableDeclaration& field);

public:
    ExpressionTypeResolver(
        ScopeStack& scopeStack,
        vector<string>& errors
    );

    const TypeInfo* resolve(const Expression& expression);
};

#endif
