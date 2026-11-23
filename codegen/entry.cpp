#include "codegen/emitter.h"

#include <stdexcept>

namespace codegen {

namespace {

std::string argvAt(Function& function, size_t index) {
    const auto slot = function.temporary("getelementptr ptr, ptr %argv, i64 " +
                                         std::to_string(index));
    return function.temporary("load ptr, ptr " + slot);
}

Value parseArgument(Function& function, const Type& type, const std::string& text) {
    switch (type.kind) {
    case TypeKind::Integer:
        return {&type, function.temporary("call i32 @imp.parse.integer(ptr " + text + ")")};
    case TypeKind::Real:
        return {&type, function.temporary("call double @imp.parse.real(ptr " + text + ")")};
    case TypeKind::Boolean:
        return {&type, function.temporary("call i1 @imp.parse.boolean(ptr " + text + ")")};
    default:
        throw std::logic_error("cannot parse an aggregate launch argument");
    }
}

bool supportsLaunch(const Routine& routine) {
    for (const auto* type : routine.parameters) {
        if (type->kind == TypeKind::Array || type->kind == TypeKind::Record) return false;
    }
    return true;
}

void emitLaunch(Function& entry, const Routine& routine) {
    // argv[0] is the executable, argv[1] is the routine name.
    std::string args;
    for (size_t i = 0; i < routine.parameters.size(); ++i) {
        const auto text = argvAt(entry, i + 2);
        const auto argument = parseArgument(entry, *routine.parameters[i], text);
        if (i != 0) args += ", ";
        args += argument.typed();
    }
    entry.emit("call void @imp.initialize()");
    entry.emit("call " + routine.result->ir + " " + routine.ir + "(" + args + ")");
    entry.terminate("ret i32 0");
}

} // namespace

const Routine* Module::findDefaultRoutine() const {
    const Routine* first = nullptr;
    const Routine* test = nullptr;
    for (const auto& name : entryNames) {
        const auto& routine = routines.at(name);
        if (!routine.parameters.empty()) continue;
        if (name == "main") return &routine;
        if (name == "test") test = &routine;
        if (!first) first = &routine;
    }
    return test ? test : first;
}

void Module::emitNamedEntries(Function& entry, const std::string& invalid) {
    const auto requestedName = argvAt(entry, 1);
    for (const auto& name : entryNames) {
        const auto& routine = routines.at(name);
        if (!supportsLaunch(routine)) continue;
        const auto invoke = entry.label("invoke");
        const auto next = entry.label("next");
        const auto nameConstant = stringConstant(name);
        const auto comparison = entry.temporary("call i32 @strcmp(ptr " + requestedName +
                                                 ", ptr " + nameConstant + ")");
        const auto sameName = entry.temporary("icmp eq i32 " + comparison + ", 0");
        const auto expectedArgc = std::to_string(routine.parameters.size() + 2);
        const auto sameCount = entry.temporary("icmp eq i32 %argc, " + expectedArgc);
        const auto matches = entry.temporary("and i1 " + sameName + ", " + sameCount);
        entry.branch(matches, invoke, next);

        entry.block(invoke);
        emitLaunch(entry, routine);
        entry.block(next);
    }
    entry.jump(invalid);
}

void Module::emitEntry() {
    Function entry(*this, &integer);
    const auto defaultEntry = entry.label("default");
    const auto namedEntry = entry.label("select");
    const auto invalid = entry.label("invalid");

    const auto hasName = entry.temporary("icmp sgt i32 %argc, 1");
    entry.branch(hasName, namedEntry, defaultEntry);

    entry.block(defaultEntry);
    if (const auto* routine = findDefaultRoutine()) {
        emitLaunch(entry, *routine);
    } else {
        entry.jump(invalid);
    }

    entry.block(namedEntry);
    emitNamedEntries(entry, invalid);

    entry.block(invalid);
    const auto message = stringConstant("Specify a routine name and its scalar arguments.");
    entry.emit("call i32 @puts(ptr " + message + ")");
    entry.terminate("ret i32 1");
    functions << entry.finish("i32 @main(i32 %argc, ptr %argv)");
}

} // namespace codegen
