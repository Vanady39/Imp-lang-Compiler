#ifndef SEMANTIC_TRAVERSAL_AST_TRAVERSAL_H
#define SEMANTIC_TRAVERSAL_AST_TRAVERSAL_H

#include "parser/ast.h"
#include "semantic/types/scope_stack.h"

#include <memory>

using namespace std;

class IAstTraversalObserver {
public:
    virtual ~IAstTraversalObserver() = default;

    virtual void enter(const Node& node, const ScopeStack& scopes) = 0;
    virtual void leave(const Node& node, const ScopeStack& scopes) = 0;
};

class IAstTraversal {
public:
    virtual ~IAstTraversal() = default;

    virtual void traverse(
        const Program& program,
        IAstTraversalObserver& observer
    ) = 0;

    virtual const ScopeStack& get_scopes() const = 0;
};

unique_ptr<IAstTraversal> makeAstTraversal();

#endif
