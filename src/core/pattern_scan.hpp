#pragma once

#include <Windows.h>
#include <cstdint>
#include <string>
#include <vector>

namespace got {

struct PatternSpec {
    const char* name;
    const char* pattern;
    bool ripRelative;
    int ripOffset;
    int ripInstructionSize;
};

std::vector<int> ParsePattern(const char* pattern);
uintptr_t ScanModule(const wchar_t* moduleName, const char* pattern);
uintptr_t ResolveRip(uintptr_t instruction, int offset, int instructionSize);

}  // namespace got
