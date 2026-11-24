#include "codegen/generator.h"

#include <llvm-c/Analysis.h>
#include <llvm-c/BitWriter.h>
#include <llvm-c/Core.h>
#include <llvm-c/IRReader.h>

#include <memory>
#include <stdexcept>

std::string makeLlvmBitcode(const std::string& ir) {
    std::unique_ptr<LLVMOpaqueContext, decltype(&LLVMContextDispose)> context(
        LLVMContextCreate(), LLVMContextDispose);
    // LLVMParseIRInContext takes ownership of the input memory buffer.
    auto input = LLVMCreateMemoryBufferWithMemoryRange(ir.data(), ir.size(), "Imp", true);
    LLVMModuleRef parsed = nullptr;
    char* diagnostic = nullptr;
    if (LLVMParseIRInContext(context.get(), input, &parsed, &diagnostic)) {
        std::unique_ptr<char, decltype(&LLVMDisposeMessage)> message(diagnostic, LLVMDisposeMessage);
        throw std::runtime_error("LLVM IR parsing failed:\n" + std::string(message.get()));
    }

    std::unique_ptr<LLVMOpaqueModule, decltype(&LLVMDisposeModule)> module(parsed, LLVMDisposeModule);
    const auto invalid = LLVMVerifyModule(module.get(), LLVMReturnStatusAction, &diagnostic);
    std::unique_ptr<char, decltype(&LLVMDisposeMessage)> message(diagnostic, LLVMDisposeMessage);
    if (invalid) {
        throw std::runtime_error("LLVM IR verification failed:\n" + std::string(message.get()));
    }
    std::unique_ptr<LLVMOpaqueMemoryBuffer, decltype(&LLVMDisposeMemoryBuffer)> bitcode(
        LLVMWriteBitcodeToMemoryBuffer(module.get()), LLVMDisposeMemoryBuffer);
    return {LLVMGetBufferStart(bitcode.get()), LLVMGetBufferSize(bitcode.get())};
}
