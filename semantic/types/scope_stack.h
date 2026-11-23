#ifndef SEMANTIC_TYPES_SCOPE_STACK_H
#define SEMANTIC_TYPES_SCOPE_STACK_H

#include "semantic/types/symbol.h"

#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

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

    Symbol* lookup(const string& name) {
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
            auto found = it->find(name);
            if (found != it->end()) {
                return &found->second;
            }
        }
        return nullptr;
    }

    bool declare(const Symbol& symbol) {
        if (scopes.empty()) {
            return false;
        }
        return scopes.back().emplace(symbol.name, symbol).second;
    }

    Symbol* currentScopeFind(const string& name) {
        if (scopes.empty()) {
            return nullptr;
        }
        auto found = scopes.back().find(name);
        return found == scopes.back().end() ? nullptr : &found->second;
    }

    void rememberDeclarationType(const VariableDeclaration& declaration, const TypeInfo& type) {
        declarationTypes.insert_or_assign(&declaration, &type);
    }

    const TypeInfo* declarationType(const VariableDeclaration& declaration) const {
        const auto found = declarationTypes.find(&declaration);
        return found == declarationTypes.end() ? nullptr : found->second;
    }

private:
    vector<unordered_map<string, Symbol>> scopes;
    // Record field types are still needed after their declaration scopes close.
    unordered_map<const VariableDeclaration*, const TypeInfo*> declarationTypes;
};

#endif
