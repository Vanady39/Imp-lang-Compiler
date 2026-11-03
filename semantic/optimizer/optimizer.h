#ifndef SEMANTIC_OPTIMIZER_OPTIMIZER_H
#define SEMANTIC_OPTIMIZER_OPTIMIZER_H

#include "parser/ast.h"

#include <memory>

using namespace std;

class IOptimizer {
public:
    virtual ~IOptimizer() = default;

    virtual void optimize(Program& program) = 0;
};

unique_ptr<IOptimizer> makeOptimizer();

#endif
