#include "features/item_features.hpp"

#include <Windows.h>

#include "core/game_pointers.hpp"
#include "core/log.hpp"
#include "core/notify.hpp"
#include "core/trainer_state.hpp"

namespace got {

namespace {
using AddResourceFn = void(__fastcall*)(uintptr_t pInventory, uint32_t resourceId, uint32_t amount);

// See the NOTE in item_features.hpp: verify this signature against your
// own scan notes before shipping if the game updates.
using AddGearFn = void(__fastcall*)(uintptr_t pGear, uint64_t gearHash, uintptr_t pPermGear);
}  // namespace

void InitItemFeatures() {
    Log("[Items] Initialized Items feature module.");
}

void ShutdownItemFeatures() {
}

bool AddResourceToInventory(uint32_t resourceId, uint32_t amount) {
    const uintptr_t pInventory = GetGamePointers().inventoryInstance;
    const uintptr_t pAddFn = GetGamePointers().addResourceFunction;

    if (!pInventory || !pAddFn) {
        Log("[Items] ERROR: Inventory pointer or AddResource fn not found!");
        PushToast(ToastLevel::Error, "Add Resource failed: pointers not scanned");
        return false;
    }

    if (IsBadReadPtr(reinterpret_cast<void*>(pInventory), 0x100)) {
        Log("[Items] ERROR: pInventory (0x%llX) memory not readable.", static_cast<unsigned long long>(pInventory));
        PushToast(ToastLevel::Error, "Add Resource failed: bad pointer");
        return false;
    }

    Log("[Items] Calling AddResource(pInventory=0x%llX, ID=%u, Amount=%u)...",
        static_cast<unsigned long long>(pInventory), resourceId, amount);

    auto fn = reinterpret_cast<AddResourceFn>(pAddFn);
    fn(pInventory, resourceId, amount);
    Log("[Items] AddResource executed successfully.");
    PushToast(ToastLevel::Success, "Resource added");
    return true;
}

bool GiveGear(uint64_t gearHash) {
    const auto& ptrs = GetGamePointers();

    if (!ptrs.addGearFunction || !ptrs.gearInstance || !ptrs.templateList || !ptrs.permGear) {
        Log("[Items] ERROR: Give Gear requires pGear + pTemplates + pPermGear + AddGear to all be scanned.");
        PushToast(ToastLevel::Error, "Give Gear failed: pointers not scanned");
        return false;
    }

    if (IsBadReadPtr(reinterpret_cast<void*>(ptrs.gearInstance), sizeof(uintptr_t))) {
        Log("[Items] ERROR: pGear (0x%llX) memory not readable.", static_cast<unsigned long long>(ptrs.gearInstance));
        PushToast(ToastLevel::Error, "Give Gear failed: bad pointer");
        return false;
    }

    Log("[Items] Calling AddGear(pGear=0x%llX, hash=%016llX, pPermGear=0x%llX)...",
        static_cast<unsigned long long>(ptrs.gearInstance),
        static_cast<unsigned long long>(gearHash),
        static_cast<unsigned long long>(ptrs.permGear));

    auto fn = reinterpret_cast<AddGearFn>(ptrs.addGearFunction);
    fn(ptrs.gearInstance, gearHash, ptrs.permGear);
    Log("[Items] AddGear executed.");
    PushToast(ToastLevel::Success, "Gear granted");
    return true;
}

void UpdateItemFeatures() {
}

}  // namespace got
