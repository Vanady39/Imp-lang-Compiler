#ifndef CODEGEN_EMITTER_H
#define CODEGEN_EMITTER_H

#include "codegen/generator.h"
#include "shared/types/type_info.h"

#include <deque>
#include <optional>
#include <sstream>
#include <unordered_map>

namespace codegen {

struct Type;

struct Value {
    const Type* type;
    std::string ir;
    std::string typed() const;
};

struct Binding {
    Value value;
    bool isType = false;
};

using Scope = std::unordered_map<std::string, Binding>;

struct Field {
    const VariableDeclaration* declaration;
    const Type* type;
};

struct Type {
    TypeKind kind;
    std::string ir;
    const Type* element = nullptr;
    std::optional<int> extent{};
    std::string layout{};
    std::vector<Field> fields{};
    Scope initializerScope{};
};

struct Routine {
    std::string ir;
    const Type* result;
    std::vector<const Type*> parameters;
};

[[noreturn]] void fail(const Node& node, const std::string& message);

class Module;

class Function {
    Module& module;
    const Type* result;
    std::vector<Scope> scopes;
    const Scope* initializerScope = nullptr;
    std::ostringstream allocations;
    std::ostringstream instructions;
    unsigned nextId = 0;
    bool terminated = false;

    const Binding* findBinding(const std::string& name) const;
    const Binding& lookup(const std::string& name, const Node& node) const;
    Scope visibleBindings() const;
    Value typeInitializer(const Expression& expr, const Scope& scope);
    const Type* typeOf(const Expression& expr) const;
    size_t fieldIndex(const Type& type, const MemberAccess& expr) const;
    Value address(const Expression& expr);
    Value call(const RoutineCall& expr);
    Value binary(const BinaryExpression& expr);
    Value convert(Value value, const Type* target, const Node& node);
    Value load(const Value& address);
    void store(const Value& value, const Value& address);
    Value slot(const Type* type);
    Value defaultValue(const Type* type, const Node& node);
    std::string allocate(const std::string& type, const std::string& count);
    Value arrayLength(const Value& array);
    Value arrayElement(const Value& array, const Value& index);
    Value recordField(const Value& record, size_t index);
    void guard(const std::string& condition);
    void emitNode(const Node& node);
    void emitIf(const IfStatement& node);
    void emitWhile(const WhileLoop& node);
    void emitFor(const ForLoop& node);
    void emitPrint(const PrintStatement& node);
    void emitReturn(const Expression& expr);

public:
    Function(Module& module, const Type* result);

    const Type* resolveType(const TypeNode& node);
    Value expression(const Expression& expr);
    void variable(const VariableDeclaration& node, bool global = false);
    void typeDeclaration(const TypeDeclaration& node, bool global = false);
    std::string routine(const RoutineDeclaration& node, const Routine& signature);
    void body(const Body& node);
    std::string finish(const std::string& signature);

    void emit(const std::string& line);
    std::string temporary(const std::string& instruction);
    std::string label(const std::string& name);
    void block(const std::string& name);
    void jump(const std::string& target);
    void branch(const std::string& condition, const std::string& yes, const std::string& no);
    void terminate(const std::string& instruction);
};

class Module {
    std::ostringstream globals;
    std::ostringstream functions;
    std::deque<Type> types;
    std::unordered_map<std::string, std::string> strings;
    std::vector<std::string> entryNames;

    void declareRoutine(Function& global, const RoutineDeclaration& node);
    const Routine* findDefaultRoutine() const;
    void emitNamedEntries(Function& entry, const std::string& invalid);
    void emitEntry();

public:
    const Type integer{TypeKind::Integer, "i32"};
    const Type real{TypeKind::Real, "double"};
    const Type boolean{TypeKind::Boolean, "i1"};
    const Type voidType{TypeKind::Error, "void"};
    Scope scope;
    std::unordered_map<std::string, Routine> routines;

    const Type* addType(Type type);
    std::string addGlobal(const Type* type);
    std::string stringConstant(const std::string& value);
    std::string generate(const Program& program);
};

} // namespace codegen

#endif
