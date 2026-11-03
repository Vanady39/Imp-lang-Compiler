#ifndef SEMANTIC_OPTIMIZER_EXPRESSION_OPTIMIZER_H
#define SEMANTIC_OPTIMIZER_EXPRESSION_OPTIMIZER_H

#include "parser/ast.h"

namespace optimizer_detail {

ExpressionPtr optimizeExpression(ExpressionPtr expression);

} // namespace optimizer_detail

#endif
