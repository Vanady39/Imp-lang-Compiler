#include "semantic/checker/checker.h"
#include "semantic/types/common.h"

#include <stdexcept>
#include <type_traits>

static_assert(std::has_virtual_destructor_v<IChecker>);
static_assert(std::is_same_v<decltype(std::declval<const IChecker&>().get_errors()),
                             const std::vector<std::string>&>);

void require(bool condition) {
    if (!condition) throw std::runtime_error("semantic base regression");
}

int main() {
    require(TypeInfo{}.kind == TypeKind::Error);
    require(sameType(integerType, integerType));
    require(!sameType(integerType, realType));
    require(!sameType(errorType, errorType));
    require(!sameType(TypeInfo{TypeKind::Array}, TypeInfo{TypeKind::Array}));
    require(!sameType(TypeInfo{TypeKind::Record}, TypeInfo{TypeKind::Record}));
    auto array = arrayType(integerType);
    require(sameType(array, arrayType(integerType)));
    require(!sameType(array, arrayType(realType)));
    TypeDeclaration first(Span(1, 0, 0), "First", nullptr);
    TypeDeclaration second(Span(1, 0, 0), "Second", nullptr);
    require(sameType(recordType(first), recordType(first)));
    require(!sameType(recordType(first), recordType(second)));

    ScopeStack scopes;
    Symbol outer{"x", SymbolKind::Variable, Span(1, 0, 0), &integerType, false, {}};
    Symbol inner{"x", SymbolKind::Variable, Span(1, 0, 0), &realType, true, {}};
    require(scopes.empty() && scopes.depth() == 0);
    require(!scopes.pop());
    require(!scopes.declare(outer));
    require(scopes.lookup("x") == nullptr);
    require(scopes.currentScopeFind("x") == nullptr);
    scopes.push();
    require(scopes.declare(outer));
    require(!scopes.declare(inner));
    scopes.push();
    require(scopes.currentScopeFind("x") == nullptr);
    require(scopes.lookup("x")->type == &integerType);
    require(scopes.declare(inner));
    require(scopes.lookup("x")->readOnly);
    require(scopes.lookup("x")->type == &realType);
    require(scopes.pop());
    require(scopes.lookup("x")->type == &integerType);
    require(scopes.pop() && scopes.empty());
}
