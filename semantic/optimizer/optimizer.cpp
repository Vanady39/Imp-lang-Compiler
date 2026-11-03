#include "semantic/optimizer/optimizer.h"

#include "semantic/optimizer/node_optimizer/node_optimizer.h"

using namespace std;

namespace {

class Optimizer final : public IOptimizer {
public:
    void optimize(Program& program) override {
        // Программа владеет декларациями, поэтому каждая декларация
        // временно переводится в общий NodePtr для единого рекурсивного API.
        for (auto& declaration : program.declarations) {
            NodePtr node = std::move(declaration);
            optimizer_detail::optimizeNode(node);
            declaration = unique_ptr<Declaration>(
                static_cast<Declaration*>(node.release())
            );
        }
    }
};

} // namespace

unique_ptr<IOptimizer> makeOptimizer() {
    return make_unique<Optimizer>();
}
