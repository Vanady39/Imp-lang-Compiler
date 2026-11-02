#ifndef SEMANTIC_CHECKER_WHILE_LOOP
#define SEMANTIC_CHECKER_WHILE_LOOP

#include "parser/ast.h"
#include "semantic/types/scope_stack.h"

#include <memory>
#include <string>
#include <vector>

using namespace std;

class ICheckerWhile {
protected:
    vector<string>& errors;

public:
    explicit ICheckerWhile(vector<string>& errors)
        : errors(errors) {}

    virtual ~ICheckerWhile() = default;

    virtual bool checkCondition(const WhileLoop& whileLoop) = 0;
};

unique_ptr<ICheckerWhile> makeCheckerWhile(
    ScopeStack& scopes,
    vector<string>& errors
);

#endif