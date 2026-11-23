#include "parser/parser.h"
#include "semantic/checker/checker.h"
#include "semantic/optimizer/optimizer.h"
#include "semantic/types/common.h"

#include <stdexcept>
#include <type_traits>

static_assert(std::has_virtual_destructor_v<IChecker>);
static_assert(std::is_same_v<decltype(std::declval<const IChecker&>().get_errors()),
                             const std::vector<std::string>&>);

void require(bool condition) {
    if (!condition) throw std::runtime_error("semantic base regression");
}

void testConstantIfScopes() {
    auto lexer = makeLexer(R"(routine shadow() is
    var a : boolean is false
    if true then
        var a : real is 2.5
        a := 3.5
        print a + 0
    end
    print a = false
end

routine earlyReturn() : integer is
    if true then
        if false then
            print 1
        else
            return 2
        end
        print 3
    end
    print 4
end
)");
    auto parser = makeParser(lexer->GetBuffer("constant-if"));
    auto program = parser->parseProgram();
    require(lexer->GetErrors().empty() && parser->GetErrors().empty());

    auto checker = makeChecker();
    checker->check(*program);
    require(checker->get_errors().empty());
    makeOptimizer()->optimize(*program);
    const auto result = checker->check(*program);
    require(checker->get_errors().empty());

    const auto& shadow = static_cast<const RoutineDeclaration&>(*program->declarations.at(0));
    require(shadow.body->items.size() == 3);
    const auto& branch = dynamic_cast<const Body&>(*shadow.body->items.at(1));
    const auto& innerPrint = dynamic_cast<const PrintStatement&>(*branch.items.at(2));
    const auto& outerPrint = dynamic_cast<const PrintStatement&>(*shadow.body->items.at(2));
    require(result.expressionTypes.at(innerPrint.values.at(0).get()).kind == TypeKind::Real);
    require(result.expressionTypes.at(outerPrint.values.at(0).get()).kind == TypeKind::Boolean);

    const auto& earlyReturn = static_cast<const RoutineDeclaration&>(*program->declarations.at(1));
    require(earlyReturn.body->items.size() == 1);
    const auto& thenBlock = dynamic_cast<const Body&>(*earlyReturn.body->items.at(0));
    require(thenBlock.items.size() == 1);
    const auto& elseBlock = dynamic_cast<const Body&>(*thenBlock.items.at(0));
    require(elseBlock.items.size() == 1);
    require(elseBlock.items.at(0)->nodeType == NodeType::ReturnStatement);
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

    testConstantIfScopes();
}
