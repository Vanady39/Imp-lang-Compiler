#ifndef SEMANTIC_TRAVERSAL_AST_TRAVERSAL_H
#define SEMANTIC_TRAVERSAL_AST_TRAVERSAL_H

#include "parser/ast.h"
#include "semantic/types/scope_stack.h"

#include <memory>

using namespace std;

class IAstTraversalObserver {
public:
    virtual ~IAstTraversalObserver() = default;

    // Событие вызывается в текущей внешней области видимости узла.
    virtual void enter(const Node& node, ScopeStack& scopes) = 0;

    // Событие вызывается после обхода всех дочерних узлов.
    virtual void leave(const Node& node, ScopeStack& scopes) = 0;

    // События обозначают время жизни специальной области конструкции.
    virtual void enterScope(const Node& node, ScopeStack& scopes) {
        (void)node;
        (void)scopes;
    }

    virtual void leaveScope(const Node& node, ScopeStack& scopes) {
        (void)node;
        (void)scopes;
    }
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

unique_ptr<IAstTraversal> makeAstTraversal(ScopeStack& scopes);

#endif
