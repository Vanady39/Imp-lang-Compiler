#include "semantic/optimizer/body_optimizer/body_optimizer.h"

#include "semantic/optimizer/node_optimizer/node_optimizer.h"

using namespace std;

namespace optimizer_detail {
namespace {

void appendBodyItems(
    Body& body,
    vector<NodePtr>& optimized,
    bool& reachable
) {
    for (auto& item : body.items) {
        if (!reachable) {
            break;
        }
        if (item == nullptr) {
            continue;
        }

        if (item->nodeType == NodeType::ReturnStatement) {
            reachable = false;
        }
        optimized.push_back(std::move(item));
    }
}

} // namespace

// Один Body оптимизируется слева направо. После return все следующие
// узлы недостижимы и удаляются из этого блока.
void optimizeBody(Body& body) {
    vector<NodePtr> optimized;
    bool reachable = true;

    for (auto& item : body.items) {
        if (!reachable) {
            break;
        }
        if (item == nullptr) {
            continue;
        }

        optimizeNode(item);
        if (item == nullptr) {
            continue;
        }

        // После упрощения if выбранная ветка представлена Body. Её
        // элементы нужно встроить в родительский блок без лишнего scope.
        if (item->nodeType == NodeType::Body) {
            // Выбранная ветка if не должна оставлять искусственный Body
            // в результате, иначе изменится структура областей видимости.
            auto selected = unique_ptr<Body>(
                static_cast<Body*>(item.release())
            );
            appendBodyItems(*selected, optimized, reachable);
            continue;
        }

        if (item->nodeType == NodeType::ReturnStatement) {
            reachable = false;
        }
        optimized.push_back(std::move(item));
    }

    body.items = std::move(optimized);
}

} // namespace optimizer_detail
