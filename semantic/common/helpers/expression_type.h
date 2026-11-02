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
        const Node& node,
        const string& message
    );

    const VariableDeclaration* findField(
        const TypeInfo& type,
        const string& member
    ) const;

    const TypeInfo* resolvePrimitiveType(const PrimitiveType& type);
    const TypeInfo* resolveNamedType(const NamedType& type);
    const TypeInfo* resolveArrayType(const ArrayType& type);
    const TypeInfo* resolveRecordType(
        const RecordType& type,
        const TypeDeclaration* declaration
    );
    const TypeInfo* resolveType(
        const TypeNode& type,
        const TypeDeclaration* declaration = nullptr
    );
    const TypeInfo* resolveFieldType(const VariableDeclaration& field);

public:
    ExpressionTypeResolver(
        ScopeStack& scopeStack,
        vector<string>& errors
    );

    const TypeInfo* resolve(const Expression& expression);
};

#endif
