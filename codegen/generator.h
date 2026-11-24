#ifndef CODEGEN_GENERATOR_H
#define CODEGEN_GENERATOR_H

#include "parser/ast.h"

#include <string>

// Takes a successfully checked and optimized program.
std::string generateLlvmIr(const Program& program);
std::string makeLlvmBitcode(const std::string& ir);

#endif
