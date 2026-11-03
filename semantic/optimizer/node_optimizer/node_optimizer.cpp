#include "semantic/optimizer/node_optimizer/node_optimizer.h"

#include "semantic/optimizer/body_optimizer/body_optimizer.h"
#include "semantic/optimizer/expression_optimizer/expression_optimizer.h"

using namespace std;

namespace optimizer_detail {
namespace {

void simplifyIf(NodePtr& node) {
    auto* statement = static_cast<IfStatement*>(node.get());
    if (statement->condition == nullptr ||
        statement->condition->nodeType != NodeType::BooleanLiteral) {
        return;
    }

    auto* condition = static_cast<BooleanLiteral*>(statement->condition.get());
    BodyPtr selected = condition->value
        ? std::move(statement->thenBody)
        : std::move(statement->elseBody);
    if (selected == nullptr) {
        node.reset();
        return;
    }

    // Временный Body нужен, чтобы передать выбранную ветку наружу. На
    // уровне родительского Body он сразу раскрывается в список items.
    node = make_unique<Body>(selected->span, std::move(selected->items));
}

} // namespace

// Рекурсивно обновляет владение дочерними unique_ptr и применяет
// локальное упрощение для текущего узла.
void optimizeNode(NodePtr& node) {
    if (node == nullptr) {
        return;
    }

    switch (node->nodeType) {
    case NodeType::VariableDeclaration: {
        auto* variable = static_cast<VariableDeclaration*>(node.get());
        variable->initializer = optimizeExpression(
            std::move(variable->initializer)
        );
        break;
    }
    case NodeType::RoutineDeclaration: {
        auto* routine = static_cast<RoutineDeclaration*>(node.get());
        if (routine->body != nullptr) {
            optimizeBody(*routine->body);
        }
        routine->expressionBody = optimizeExpression(
            std::move(routine->expressionBody)
        );
        break;
    }
    case NodeType::Body:
        optimizeBody(*static_cast<Body*>(node.get()));
        break;
    case NodeType::Assignment: {
        auto* assignment = static_cast<Assignment*>(node.get());
        assignment->target = optimizeExpression(std::move(assignment->target));
        assignment->value = optimizeExpression(std::move(assignment->value));
        break;
    }
    case NodeType::CallStatement: {
        auto* call = static_cast<CallStatement*>(node.get());
        ExpressionPtr optimized = optimizeExpression(std::move(call->call));
        call->call.reset(static_cast<RoutineCall*>(optimized.release()));
        break;
    }
    case NodeType::WhileLoop: {
        auto* loop = static_cast<WhileLoop*>(node.get());
        loop->condition = optimizeExpression(std::move(loop->condition));
        if (loop->body != nullptr) {
            optimizeBody(*loop->body);
        }
        break;
    }
    case NodeType::ForLoop: {
        auto* loop = static_cast<ForLoop*>(node.get());
        loop->rangeStart = optimizeExpression(std::move(loop->rangeStart));
        loop->rangeEnd = optimizeExpression(std::move(loop->rangeEnd));
        if (loop->body != nullptr) {
            optimizeBody(*loop->body);
        }
        break;
    }
    case NodeType::IfStatement: {
        auto* statement = static_cast<IfStatement*>(node.get());
        statement->condition = optimizeExpression(std::move(statement->condition));
        if (statement->thenBody != nullptr) {
            optimizeBody(*statement->thenBody);
        }
        if (statement->elseBody != nullptr) {
            optimizeBody(*statement->elseBody);
        }
        simplifyIf(node);
        break;
    }
    case NodeType::PrintStatement: {
        auto* print = static_cast<PrintStatement*>(node.get());
        for (auto& value : print->values) {
            value = optimizeExpression(std::move(value));
        }
        break;
    }
    case NodeType::ReturnStatement: {
        auto* statement = static_cast<ReturnStatement*>(node.get());
        statement->value = optimizeExpression(std::move(statement->value));
        break;
    }
    default:
        break;
    }
}

} // namespace optimizer_detail
