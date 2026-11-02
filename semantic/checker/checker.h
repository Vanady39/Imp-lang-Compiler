#ifndef SEMANTIC_CHECKER_CHECKER_H
#define SEMANTIC_CHECKER_CHECKER_H

#include "parser/ast.h"
#include "shared/types/type_info.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

struct CheckResult {
    unordered_map<Expression*, TypeInfo> expressionTypes;
};

class IChecker {
public:
    virtual ~IChecker() = default;
    virtual CheckResult check(Program& program) = 0;
    virtual const vector<string>& get_errors() const = 0;
};

unique_ptr<IChecker> makeChecker();

#endif
