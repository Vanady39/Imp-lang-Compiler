#ifndef SEMANTIC_OPTIMIZER_NODE_OPTIMIZER_H
#define SEMANTIC_OPTIMIZER_NODE_OPTIMIZER_H

#include "parser/ast.h"

namespace optimizer_detail {

void optimizeNode(NodePtr& node);

} // namespace optimizer_detail

#endif
