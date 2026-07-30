#include "core/pattern_scan.hpp"

#include <Psapi.h>
#include <algorithm>
#include <sstream>

namespace got {

std::vector<int> ParsePattern(const char* pattern) {
    std::vector<int> bytes;
    std::istringstream stream(pattern);
    std::string token;
    while (stream >> token) {
        if (token == "?" || token == "??") {
            bytes.push_back(-1);
        } else {
            bytes.push_back(static_cast<int>(std::stoul(token, nullptr, 16)));
        }
    }
    return bytes;
}

uintptr_t ScanModule(const wchar_t* moduleName, const char* pattern) {
    HMODULE module = GetModuleHandleW(moduleName);
    if (!module) {
        return 0;
    }

    MODULEINFO info{};
    if (!GetModuleInformation(GetCurrentProcess(), module, &info, sizeof(info))) {
        return 0;
    }

    const auto sig = ParsePattern(pattern);
    if (sig.empty()) {
        return 0;
    }

    const auto* base = static_cast<const uint8_t*>(info.lpBaseOfDll);
    const size_t size = info.SizeOfImage;
    const size_t sigLen = sig.size();

    for (size_t i = 0; i + sigLen <= size; ++i) {
        bool match = true;
        for (size_t j = 0; j < sigLen; ++j) {
            if (sig[j] != -1 && base[i + j] != static_cast<uint8_t>(sig[j])) {
                match = false;
                break;
            }
        }
        if (match) {
            return reinterpret_cast<uintptr_t>(base + i);
        }
    }
    return 0;
}

uintptr_t ResolveRip(uintptr_t instruction, int offset, int instructionSize) {
    const auto rel = *reinterpret_cast<int32_t*>(instruction + offset);
    return instruction + instructionSize + rel;
}

}  // namespace got
