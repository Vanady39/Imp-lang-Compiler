#include "semantic/optimizer/body_optimizer/body_optimizer.h"

#include "semantic/optimizer/node_optimizer/node_optimizer.h"

using namespace std;

namespace optimizer_detail {
namespace {

bool endsWithReturn(const Node& node) {
    if (node.nodeType == NodeType::Body) {
        const auto& items = static_cast<const Body&>(node).items;
        return !items.empty() && endsWithReturn(*items.back());
    }
    return node.nodeType == NodeType::ReturnStatement;
}

} // namespace

// Один Body оптимизируется слева направо. После return все следующие
// узлы недостижимы и удаляются из этого блока.
void optimizeBody(Body& body) {
    vector<NodePtr> optimized;
    for (auto& item : body.items) {
        optimizeNode(item);
        if (item == nullptr) {
            continue;
        }

        // Выбранная ветка if сохраняет свою область видимости.
        const bool returns = endsWithReturn(*item);
        optimized.push_back(std::move(item));
        if (returns) {
            break;
        }
    }

    body.items = std::move(optimized);
}

} // namespace optimizer_detail
