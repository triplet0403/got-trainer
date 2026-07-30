#include "core/game_pointers.hpp"

#include "core/log.hpp"
#include "core/offsets.hpp"
#include "core/pattern_scan.hpp"

namespace got {

namespace {

ScanResult g_results[] = {
    {"pPlayer", false, 0},
    {"pInventory", false, 0},
    {"addResourceCall", false, 0},
    {"pGear", false, 0},
    {"pTemplates", false, 0},
    {"addGear", false, 0},
    {"pPermGear", false, 0},
    {"pCamRot", false, 0},
};

void SetResult(size_t idx, bool ok, uintptr_t addr) {
    g_results[idx].ok = ok;
    g_results[idx].address = addr;
    if (ok) {
        Log("[Scan] %-20s -> 0x%llX", g_results[idx].name.c_str(), static_cast<unsigned long long>(addr));
    } else {
        Log("[Scan] %-20s -> FAILED", g_results[idx].name.c_str());
    }
}

}  // namespace

bool GamePointers::allRootScanned() const {
    return playerInstance && inventoryInstance && addResourceFunction && gearInstance &&
           templateList && addGearFunction && permGear;
}

void GamePointers::Rescan() {
    playerInstance = 0;
    inventoryInstance = 0;
    addResourceCallSite = 0;
    addResourceFunction = 0;
    gearInstance = 0;
    templateList = 0;
    addGearFunction = 0;
    permGear = 0;
    camRotGlobal = 0;

    for (auto& r : g_results) {
        r.ok = false;
        r.address = 0;
    }

    const uintptr_t playerInstr = ScanModule(
        kGameModule,
        "48 8B 3D ?? ?? ?? ?? 45 0F 57 C9 48 85 FF 0F 84 F0 00 00 00");
    if (playerInstr) {
        playerInstance = ResolveRip(playerInstr, 3, 7);
        SetResult(0, playerInstance != 0, playerInstance);
    } else {
        SetResult(0, false, 0);
    }

    const uintptr_t addResInstr = ScanModule(
        kGameModule,
        "48 8D 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? 4D 8B CD 88 9C 24 80 00 00 00 44 8B C7");
    if (addResInstr) {
        inventoryInstance = ResolveRip(addResInstr, 3, 7);
        addResourceCallSite = addResInstr + 7;
        const int32_t rel = *reinterpret_cast<int32_t*>(addResourceCallSite + 1);
        addResourceFunction = addResourceCallSite + 5 + rel;
        SetResult(1, inventoryInstance != 0, inventoryInstance);
        SetResult(2, addResourceFunction != 0, addResourceFunction);
    } else {
        SetResult(1, false, 0);
        SetResult(2, false, 0);
    }

    const uintptr_t gearInstr = ScanModule(
        kGameModule, "48 8D 0D ?? ?? ?? ?? 8B 44 24 4C");
    if (gearInstr) {
        gearInstance = ResolveRip(gearInstr, 3, 7);
        SetResult(3, gearInstance != 0, gearInstance);
    } else {
        SetResult(3, false, 0);
    }

    const uintptr_t tmplInstr = ScanModule(
        kGameModule,
        "48 8B 05 ?? ?? ?? ?? 48 0F BF CA 48 8B 14 C8 48 85 D2");
    if (tmplInstr) {
        templateList = ResolveRip(tmplInstr, 3, 7);
        SetResult(4, templateList != 0, templateList);
    } else {
        SetResult(4, false, 0);
    }

    const uintptr_t addGear = ScanModule(
        kGameModule, "44 89 4C 24 ? 44 89 44 24 ? 48 89 54 24 ? 55");
    if (addGear) {
        addGearFunction = addGear;
        SetResult(5, true, addGear);
    } else {
        SetResult(5, false, 0);
    }

    const uintptr_t permInstr = ScanModule(
        kGameModule,
        "48 8B 15 ?? ?? ?? ?? 44 8B 64 24 44 45 33 C9 41 8B F1 45 85 E4 0F 8E 06 02 00 00");
    if (permInstr) {
        permGear = ResolveRip(permInstr, 3, 7);
        SetResult(6, permGear != 0, permGear);
    } else {
        SetResult(6, false, 0);
    }

    const uintptr_t camRotInstr = ScanModule(
        kGameModule, "48 8B 05 ?? ?? ?? ?? 0F 57 DB 0F 29 74 24 10 0F 57 F6");
    if (camRotInstr) {
        camRotGlobal = ResolveRip(camRotInstr, 3, 7);
        SetResult(7, camRotGlobal != 0, camRotGlobal);
    } else {
        SetResult(7, false, 0);
    }
}

const ScanResult* GamePointers::GetResults() const {
    return g_results;
}

size_t GamePointers::ResultCount() const {
    return sizeof(g_results) / sizeof(g_results[0]);
}

GamePointers& GetGamePointers() {
    static GamePointers instance;
    return instance;
}

}  // namespace got
