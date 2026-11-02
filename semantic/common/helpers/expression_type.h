#ifndef SEMANTIC_COMMON_HELPERS_EXPRESSION_TYPE_H
#define SEMANTIC_COMMON_HELPERS_EXPRESSION_TYPE_H

#include "semantic/types/scope_stack.h"

#include <memory>
#include <string>
#include <vector>

using namespace std;

class IExpressionTypeResolver {
protected:
    vector<string>& errors;

public:
    explicit IExpressionTypeResolver(vector<string>& errors) : errors(errors) {};
    virtual ~IExpressionTypeResolver() = default;

    virtual const TypeInfo* resolve(const Expression& expression) = 0;
    virtual const TypeInfo* resolveType(
        const TypeNode& type,
        const TypeDeclaration* declaration = nullptr
    ) = 0;
};

unique_ptr<IExpressionTypeResolver> makeExpressionTypeResolver(
    ScopeStack& scopeStack,
    vector<string>& errors
);

#endif
