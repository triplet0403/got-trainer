#pragma once

#include <cstdint>
#include <string>

namespace got {

struct ScanResult {
    std::string name;
    bool ok = false;
    uintptr_t address = 0;
};

struct GamePointers {
    uintptr_t playerInstance = 0;
    uintptr_t inventoryInstance = 0;
    uintptr_t addResourceCallSite = 0;
    uintptr_t addResourceFunction = 0;
    uintptr_t gearInstance = 0;
    uintptr_t templateList = 0;
    uintptr_t addGearFunction = 0;
    uintptr_t permGear = 0;
    uintptr_t camRotGlobal = 0;

    bool allRootScanned() const;
    void Rescan();
    const ScanResult* GetResults() const;
    size_t ResultCount() const;
};

GamePointers& GetGamePointers();

}  // namespace got
