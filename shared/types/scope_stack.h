#ifndef SHARED_TYPES_SCOPE_STACK_H
#define SHARED_TYPES_SCOPE_STACK_H

#include "lexer/lexer.h"
#include "parser/ast.h"
#include "shared/types/type_info.h"

#include <string>
#include <unordered_map>
#include <vector>

// Scoped symbol table shared by compiler stages. A stack of hash maps pushed
// and popped as the tree walk enters and leaves routines and loops: lookup
// walks innermost-outward (shadowing), declare checks only the current scope
// (duplicates).

enum class SymbolKind {
    Variable,
    Parameter,
    Routine,
    Type
};

struct Symbol {
    std::string name;
    SymbolKind kind;
    Span span; // where it was declared, for error messages
    const TypeInfo* type;
    bool readOnly; // true for the numeric for-loop variable (DECISIONS.md A10)
    // kind == Routine: every prototype/definition node of this name; the
    // analyzer checks that exactly one has a body (DECISIONS.md A9).
    std::vector<RoutineDeclaration*> routineDeclarations;
};

class ScopeStack {
public:
    void push() { scopes.emplace_back(); }

    bool empty() const { return scopes.empty(); }
    size_t depth() const { return scopes.size(); }

    bool pop() {
        if (scopes.empty()) {
            return false;
        }
        scopes.pop_back();
        return true;
    }

    // Innermost-outward lookup; nullptr when the name is not visible.
    Symbol* lookup(const std::string& name) {
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
            auto found = it->find(name);
            if (found != it->end()) {
                return &found->second;
            }
        }
        return nullptr;
    }

    // False when the name already exists in the CURRENT scope only;
    // shadowing an outer scope is allowed.
    bool declare(const Symbol& symbol) {
        if (scopes.empty()) {
            return false;
        }
        return scopes.back().emplace(symbol.name, symbol).second;
    }

    Symbol* currentScopeFind(const std::string& name) {
        if (scopes.empty()) {
            return nullptr;
        }
        auto found = scopes.back().find(name);
        return found == scopes.back().end() ? nullptr : &found->second;
    }

private:
    std::vector<std::unordered_map<std::string, Symbol>> scopes;
};

#endif
