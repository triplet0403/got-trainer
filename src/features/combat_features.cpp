#include "features/combat_features.hpp"

#include <Windows.h>

#include "core/game_pointers.hpp"
#include "core/log.hpp"
#include "core/offsets.hpp"
#include "core/pattern_scan.hpp"
#include "core/trainer_state.hpp"

namespace got {

namespace {

uintptr_t g_addrCheckBlock = 0;
uintptr_t g_addrWriteBlockRes = 0;
uintptr_t g_addrNPCState = 0;

uint8_t g_origCheckBlock[6] = {0};
uint8_t g_origWriteBlockRes[2] = {0};
uint8_t g_origNPCState[6] = {0};

bool g_patchedCheckBlock = false;
bool g_patchedWriteBlockRes = false;
bool g_patchedNPCState = false;

bool PatchMemory(uintptr_t address, const uint8_t* patchBytes, size_t size, uint8_t* originalBuffer) {
    if (!address || IsBadReadPtr(reinterpret_cast<void*>(address), size)) return false;
    DWORD oldProtect = 0;
    if (!VirtualProtect(reinterpret_cast<void*>(address), size, PAGE_EXECUTE_READWRITE, &oldProtect)) return false;
    if (originalBuffer && originalBuffer[0] == 0) {
        memcpy(originalBuffer, reinterpret_cast<void*>(address), size);
    }
    memcpy(reinterpret_cast<void*>(address), patchBytes, size);
    VirtualProtect(reinterpret_cast<void*>(address), size, oldProtect, &oldProtect);
    return true;
}

bool RestoreMemory(uintptr_t address, const uint8_t* originalBuffer, size_t size) {
    if (!address || IsBadReadPtr(reinterpret_cast<void*>(address), size)) return false;
    DWORD oldProtect = 0;
    if (!VirtualProtect(reinterpret_cast<void*>(address), size, PAGE_EXECUTE_READWRITE, &oldProtect)) return false;
    memcpy(reinterpret_cast<void*>(address), originalBuffer, size);
    VirtualProtect(reinterpret_cast<void*>(address), size, oldProtect, &oldProtect);
    return true;
}

}  // namespace

void InitCombatFeatures() {
    Log("[Combat] Scanning patterns for Auto Block, Always Parry, and NPC State...");

    g_addrCheckBlock = ScanModule(kGameModule, "0F 84 DD 01 00 00 8D 56 06 49 8B CF");
    if (g_addrCheckBlock) {
        memcpy(g_origCheckBlock, reinterpret_cast<void*>(g_addrCheckBlock), 6);
        Log("[Combat] Found aobCheckBlock at 0x%llX", static_cast<unsigned long long>(g_addrCheckBlock));
    } else {
        Log("[Combat] aobCheckBlock pattern scan FAILED.");
    }

    g_addrWriteBlockRes = ScanModule(kGameModule, "89 30 48 8B B4 24 98 00 00 00");
    if (g_addrWriteBlockRes) {
        memcpy(g_origWriteBlockRes, reinterpret_cast<void*>(g_addrWriteBlockRes), 2);
        Log("[Combat] Found aobWriteBlockRes at 0x%llX", static_cast<unsigned long long>(g_addrWriteBlockRes));
    } else {
        Log("[Combat] aobWriteBlockRes pattern scan FAILED.");
    }

    g_addrNPCState = ScanModule(kGameModule, "88 8B 98 00 00 00");
    if (g_addrNPCState) {
        memcpy(g_origNPCState, reinterpret_cast<void*>(g_addrNPCState), 6);
        Log("[Combat] Found aobNPCState at 0x%llX", static_cast<unsigned long long>(g_addrNPCState));
    } else {
        Log("[Combat] aobNPCState pattern scan FAILED.");
    }
}

void ShutdownCombatFeatures() {
    if (g_patchedCheckBlock && g_addrCheckBlock) {
        RestoreMemory(g_addrCheckBlock, g_origCheckBlock, 6);
    }
    if (g_patchedWriteBlockRes && g_addrWriteBlockRes) {
        RestoreMemory(g_addrWriteBlockRes, g_origWriteBlockRes, 2);
    }
    if (g_patchedNPCState && g_addrNPCState) {
        RestoreMemory(g_addrNPCState, g_origNPCState, 6);
    }
    g_patchedCheckBlock = false;
    g_patchedWriteBlockRes = false;
    g_patchedNPCState = false;
}

void UpdateCombatFeatures() {
    const auto& settings = GetTrainerSettings();
    const static uint8_t nops6[6] = {0x90, 0x90, 0x90, 0x90, 0x90, 0x90};
    const static uint8_t nops2[2] = {0x90, 0x90};

    const bool masterEnable = settings.masterEnable;
    const bool wantAutoBlock = masterEnable && settings.autoBlockMelee;
    const bool wantParry = masterEnable && (settings.alwaysParryOnBlock || settings.perfectParry);
    const bool wantNoIndicator = masterEnable && settings.noIndicator;

    // 1. Auto Block Melee Attacks
    if (g_addrCheckBlock) {
        if (wantAutoBlock && !g_patchedCheckBlock) {
            if (PatchMemory(g_addrCheckBlock, nops6, 6, g_origCheckBlock)) {
                g_patchedCheckBlock = true;
                Log("[Combat] Auto Block ENABLED.");
            }
        } else if (!wantAutoBlock && g_patchedCheckBlock) {
            if (RestoreMemory(g_addrCheckBlock, g_origCheckBlock, 6)) {
                g_patchedCheckBlock = false;
                Log("[Combat] Auto Block DISABLED.");
            }
        }
    }

    // 2. Always Parry on Block / Perfect Parry
    if (g_addrWriteBlockRes) {
        if (wantParry && !g_patchedWriteBlockRes) {
            if (PatchMemory(g_addrWriteBlockRes, nops2, 2, g_origWriteBlockRes)) {
                g_patchedWriteBlockRes = true;
                Log("[Combat] Always Parry / Perfect Parry ENABLED.");
            }
        } else if (!wantParry && g_patchedWriteBlockRes) {
            if (RestoreMemory(g_addrWriteBlockRes, g_origWriteBlockRes, 2)) {
                g_patchedWriteBlockRes = false;
                Log("[Combat] Always Parry / Perfect Parry DISABLED.");
            }
        }
    }

    // 3. No Indicator (NPC Detection Indicator State)
    if (g_addrNPCState) {
        if (wantNoIndicator && !g_patchedNPCState) {
            if (PatchMemory(g_addrNPCState, nops6, 6, g_origNPCState)) {
                g_patchedNPCState = true;
                Log("[Combat] No Indicator ENABLED.");
            }
        } else if (!wantNoIndicator && g_patchedNPCState) {
            if (RestoreMemory(g_addrNPCState, g_origNPCState, 6)) {
                g_patchedNPCState = false;
                Log("[Combat] No Indicator DISABLED.");
            }
        }
    }

    // 4. Force Perfect Dodge & Live Player Modifications
    if (!masterEnable) {
        return;
    }
    const uintptr_t pPlayerGlobal = GetGamePointers().playerInstance;
    if (pPlayerGlobal && !IsBadReadPtr(reinterpret_cast<void*>(pPlayerGlobal), sizeof(uintptr_t))) {
        const uintptr_t pPlayer = *reinterpret_cast<uintptr_t*>(pPlayerGlobal);
        if (pPlayer && !IsBadReadPtr(reinterpret_cast<void*>(pPlayer), 0x5000)) {
            // Stealth (Reset enemy detection float at offset 0xA8)
            if (settings.stealth) {
                float* pDetection = reinterpret_cast<float*>(pPlayer + Offsets::Detection);
                if (!IsBadReadPtr(pDetection, sizeof(float)) && *pDetection >= 0.0f && *pDetection <= 1000.0f) {
                    *pDetection = 0.0f;
                }
            }

            // Perfect Dodge byte flag at [pPlayer + 0x42B3]
            if (settings.forcePerfectDodge) {
                uint8_t* pDodgeFlag = reinterpret_cast<uint8_t*>(pPlayer + 0x42B3);
                if (!IsBadReadPtr(pDodgeFlag, sizeof(uint8_t))) {
                    *pDodgeFlag = 1;
                }
            }
        }
    }
}

}  // namespace got
