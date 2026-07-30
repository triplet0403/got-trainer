#pragma once

#include <cstdint>

namespace got {

struct TrainerVars {
    uint32_t resourceId = 0;
    uint32_t resourceAmount = 999;
    float freeCamSpeed = 25.0f;
    uint64_t gearHash = 0x5091342EE71AC35CULL;
    uint32_t pickupMultiplier = 10;
    float movementSpeedBonus = 0.02f;
};

struct TrainerSettings {
    bool menuOpen = false;

    // Global kill switch. When false, every feature below is force-disabled
    // and any active memory patches are restored, regardless of their
    // individual checkbox state. Bound to the Panic hotkey by default.
    bool masterEnable = true;

    // Player
    bool infiniteHealth = false;
    bool infiniteResolve = false;

    // Combat
    bool stealth = false;
    bool noIndicator = false;
    bool forcePerfectDodge = false;
    bool autoBlockMelee = false;
    bool alwaysParryOnBlock = false;
    bool perfectParry = false;

    // Movement
    bool movementSpeed = false;
    bool pickupMultiplier = false;

    // Camera
    bool freeCam = false;

    // Items
    bool gearUnlockOnLoad = false;

    // Misc / QoL
    bool autoSaveConfig = true;
    bool showToasts = true;
};

// One-shot actions: set to true by the UI to request an immediate action,
// consumed (and reset to false) by the owning feature module on the next
// update tick. Used for instant effects that shouldn't stay "on" forever,
// like topping off a resource once rather than continuously.
struct TrainerActions {
    bool fillHealth = false;
    bool fillResolve = false;
};

// User-configurable hotkeys (Win32 virtual-key codes). Defaults avoid
// common game bindings; all are rebindable from the Hotkeys tab.
struct TrainerHotkeys {
    int menuToggle = 0x77;      // VK_F8
    int infiniteHealth = 0x74;  // VK_F5
    int stealth = 0x75;         // VK_F6
    int freeCam = 0x76;         // VK_F7
    int panic = 0x78;           // VK_F9  -- master enable/disable
};

TrainerVars& GetTrainerVars();
TrainerSettings& GetTrainerSettings();
TrainerActions& GetTrainerActions();
TrainerHotkeys& GetTrainerHotkeys();

}  // namespace got
