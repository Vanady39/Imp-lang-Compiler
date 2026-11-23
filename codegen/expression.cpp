#include "codegen/emitter.h"

#include <bit>
#include <cstdint>
#include <iomanip>
#include <stdexcept>

namespace codegen {

namespace {

struct BinaryOpcode {
    const char* integer;
    const char* real;
    bool comparison = false;
};

BinaryOpcode binaryOpcode(const BinaryExpression& expr) {
    switch (expr.op) {
    case tokenPlus:         return {"add", "fadd"};
    case tokenMinus:        return {"sub", "fsub"};
    case tokenStar:         return {"mul", "fmul"};
    case tokenSlash:        return {"sdiv", "fdiv"};
    case tokenPercent:      return {"srem", nullptr};
    case tokenAnd:          return {"and", nullptr};
    case tokenOr:           return {"or", nullptr};
    case tokenXor:          return {"xor", nullptr};
    case tokenEqual:        return {"icmp eq", "fcmp oeq", true};
    case tokenNotEqual:     return {"icmp ne", "fcmp une", true};
    case tokenLess:         return {"icmp slt", "fcmp olt", true};
    case tokenLessEqual:    return {"icmp sle", "fcmp ole", true};
    case tokenGreater:      return {"icmp sgt", "fcmp ogt", true};
    case tokenGreaterEqual: return {"icmp sge", "fcmp oge", true};
    default: fail(expr, "unsupported binary operator");
    }
}

bool sameStorageType(const Type* a, const Type* b) {
    return a == b || (a->kind == TypeKind::Array && b->kind == TypeKind::Array &&
                      sameStorageType(a->element, b->element));
}

std::string realConstant(double number) {
    std::ostringstream text;
    text << "0x" << std::uppercase << std::hex << std::setw(16) << std::setfill('0')
         << std::bit_cast<std::uint64_t>(number);
    return text.str();
}

} // namespace

Value Function::convert(Value value, const Type* target, const Node& node) {
    if (sameStorageType(value.type, target)) return {target, std::move(value.ir)};
    if (value.type == &module.integer && target == &module.real) {
        return {target, temporary("sitofp " + value.typed() + " to double")};
    }
    if (value.type == &module.real && target == &module.integer) {
        const auto rounded = temporary("call double @llvm.round.f64(" + value.typed() + ")");
        const auto min = temporary("fcmp oge double " + rounded + ", " + realConstant(-2147483648.0));
        const auto max = temporary("fcmp ole double " + rounded + ", " + realConstant(2147483647.0));
        guard(temporary("and i1 " + min + ", " + max));
        return {target, temporary("fptosi double " + rounded + " to i32")};
    }
    if (value.type == &module.boolean && target == &module.integer) {
        return {target, temporary("zext " + value.typed() + " to i32")};
    }
    if (value.type == &module.boolean && target == &module.real) {
        return {target, temporary("uitofp " + value.typed() + " to double")};
    }
    if (value.type == &module.integer && target == &module.boolean) {
        guard(temporary("icmp ule " + value.typed() + ", 1"));
        return {target, temporary("trunc " + value.typed() + " to i1")};
    }
    fail(node, "incompatible value types");
}

size_t Function::fieldIndex(const Type& type, const MemberAccess& expr) const {
    for (size_t i = 0; i < type.fields.size(); ++i) {
        if (type.fields[i].declaration->name == expr.member) return i;
    }
    fail(expr, "unknown record field '" + expr.member + "'");
}

const Type* Function::typeOf(const Expression& expr) const {
    switch (expr.nodeType) {
    case NodeType::IntegerLiteral: return &module.integer;
    case NodeType::RealLiteral: return &module.real;
    case NodeType::BooleanLiteral: return &module.boolean;
    case NodeType::NameReference: {
        const auto& ref = static_cast<const NameReference&>(expr);
        if (!findBinding(ref.name) && module.routines.contains(ref.name)) {
            return module.routines.at(ref.name).result;
        }
        const auto& binding = lookup(ref.name, expr);
        if (binding.isType) fail(expr, "'" + ref.name + "' is a type");
        return binding.value.type;
    }
    case NodeType::BinaryExpression: {
        const auto& binary = static_cast<const BinaryExpression&>(expr);
        if (binaryOpcode(binary).comparison) return &module.boolean;
        const auto* left = typeOf(*binary.left);
        const auto* right = typeOf(*binary.right);
        return right == &module.real ? right : left;
    }
    case NodeType::UnaryExpression:
        return typeOf(*static_cast<const UnaryExpression&>(expr).operand);
    case NodeType::RoutineCall: {
        const auto& call = static_cast<const RoutineCall&>(expr);
        const auto found = module.routines.find(call.name);
        if (found == module.routines.end()) fail(expr, "unknown routine '" + call.name + "'");
        return found->second.result;
    }
    case NodeType::IndexAccess: {
        const auto* array = typeOf(*static_cast<const IndexAccess&>(expr).array);
        if (!array->element) fail(expr, "index requires an array");
        return array->element;
    }
    case NodeType::MemberAccess: {
        const auto& member = static_cast<const MemberAccess&>(expr);
        const auto* type = typeOf(*member.object);
        if (type->kind == TypeKind::Array && member.member == "length") {
            return &module.integer;
        }
        return type->fields[fieldIndex(*type, member)].type;
    }
    default:
        fail(expr, "unsupported expression");
    }
}

Value Function::address(const Expression& expr) {
    switch (expr.nodeType) {
    case NodeType::NameReference: {
        const auto& ref = static_cast<const NameReference&>(expr);
        const auto& binding = lookup(ref.name, expr);
        if (binding.isType) fail(expr, "'" + ref.name + "' is a type");
        return binding.value;
    }
    case NodeType::MemberAccess: {
        const auto& member = static_cast<const MemberAccess&>(expr);
        const auto record = expression(*member.object);
        return recordField(record, fieldIndex(*record.type, member));
    }
    case NodeType::IndexAccess: {
        const auto& index = static_cast<const IndexAccess&>(expr);
        const auto array = expression(*index.array);
        const auto value = expression(*index.index);
        if (!array.type->element || value.type != &module.integer) {
            fail(expr, "array indexing requires an array and an integer index");
        }
        return arrayElement(array, value);
    }
    default:
        fail(expr, "expected an assignable expression");
    }
}

Value Function::call(const RoutineCall& expr) {
    const auto found = module.routines.find(expr.name);
    if (found == module.routines.end()) fail(expr, "unknown routine '" + expr.name + "'");
    const auto& routine = found->second;
    if (expr.arguments.size() != routine.parameters.size()) fail(expr, "wrong argument count");
    std::string instruction = "call " + routine.result->ir + " " + routine.ir + "(";
    for (size_t i = 0; i < expr.arguments.size(); ++i) {
        if (i != 0) instruction += ", ";
        const auto& argument = *expr.arguments[i];
        instruction += convert(expression(argument), routine.parameters[i], argument).typed();
    }
    instruction += ")";
    if (routine.result == &module.voidType) {
        emit(instruction);
        return {routine.result, {}};
    }
    return {routine.result, temporary(instruction)};
}

Value Function::binary(const BinaryExpression& expr) {
    auto left = expression(*expr.left);
    auto right = expression(*expr.right);
    const auto opcode = binaryOpcode(expr);
    const bool useReal = left.type == &module.real || right.type == &module.real;
    if (useReal) {
        left = convert(left, &module.real, expr);
        right = convert(right, &module.real, expr);
    }
    const char* instruction = useReal ? opcode.real : opcode.integer;
    if (!instruction || !sameStorageType(left.type, right.type) || left.type == &module.voidType) {
        fail(expr, "incompatible operands");
    }
    if (!useReal && (expr.op == tokenSlash || expr.op == tokenPercent)) {
        // Extend before division so INT_MIN / -1 has defined wraparound.
        guard(temporary("icmp ne " + right.typed() + ", 0"));
        const auto wideLeft = temporary("sext " + left.typed() + " to i64");
        const auto wideRight = temporary("sext " + right.typed() + " to i64");
        const auto quotient = temporary(std::string(instruction) + " i64 " + wideLeft +
                                        ", " + wideRight);
        return {&module.integer, temporary("trunc i64 " + quotient + " to i32")};
    }
    return {opcode.comparison ? &module.boolean : left.type,
            temporary(std::string(instruction) + " " + left.typed() + ", " + right.ir)};
}

Value Function::expression(const Expression& expr) {
    switch (expr.nodeType) {
    case NodeType::IntegerLiteral:
        return {&module.integer, std::to_string(static_cast<const IntegerLiteral&>(expr).value)};
    case NodeType::RealLiteral:
        return {&module.real, realConstant(static_cast<const RealLiteral&>(expr).value)};
    case NodeType::BooleanLiteral:
        return {&module.boolean, static_cast<const BooleanLiteral&>(expr).value ? "true" : "false"};
    case NodeType::NameReference: {
        const auto& ref = static_cast<const NameReference&>(expr);
        if (!findBinding(ref.name) && module.routines.contains(ref.name)) {
            return call(RoutineCall(ref.span, ref.name, {}));
        }
        return load(address(expr));
    }
    case NodeType::IndexAccess:
        return load(address(expr));
    case NodeType::MemberAccess: {
        const auto& member = static_cast<const MemberAccess&>(expr);
        const auto object = expression(*member.object);
        if (object.type->kind == TypeKind::Array && member.member == "length") {
            return arrayLength(object);
        }
        return load(recordField(object, fieldIndex(*object.type, member)));
    }
    case NodeType::RoutineCall:
        return call(static_cast<const RoutineCall&>(expr));
    case NodeType::BinaryExpression:
        return binary(static_cast<const BinaryExpression&>(expr));
    case NodeType::UnaryExpression: {
        const auto& unary = static_cast<const UnaryExpression&>(expr);
        auto value = expression(*unary.operand);
        if (unary.op == tokenPlus) return value;
        if (unary.op == tokenNot) {
            value = convert(value, &module.boolean, *unary.operand);
            return {&module.boolean, temporary("xor " + value.typed() + ", true")};
        }
        if (unary.op == tokenMinus) {
            const auto instruction = value.type == &module.real
                ? "fneg " + value.typed()
                : "sub i32 0, " + value.ir;
            return {value.type, temporary(instruction)};
        }
        fail(expr, "unsupported unary operator");
    }
    default:
        fail(expr, "unsupported expression");
    }
}

} // namespace codegen
