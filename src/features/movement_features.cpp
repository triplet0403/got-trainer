#include "features/movement_features.hpp"

#include <Windows.h>

#include "core/game_pointers.hpp"
#include "core/log.hpp"
#include "core/offsets.hpp"
#include "core/trainer_state.hpp"

namespace got {

void InitMovementFeatures() {
    Log("[Movement] Initialized Movement feature module.");
}

void ShutdownMovementFeatures() {
}

void UpdateMovementFeatures() {
    const auto& settings = GetTrainerSettings();
    const auto& vars = GetTrainerVars();

    if (!settings.masterEnable || !settings.movementSpeed) {
        return;
    }

    const uintptr_t pPlayerGlobal = GetGamePointers().playerInstance;
    if (!pPlayerGlobal || IsBadReadPtr(reinterpret_cast<void*>(pPlayerGlobal), sizeof(uintptr_t))) {
        return;
    }

    const uintptr_t pPlayer = *reinterpret_cast<uintptr_t*>(pPlayerGlobal);
    if (!pPlayer || IsBadReadPtr(reinterpret_cast<void*>(pPlayer), 0x2000)) {
        return;
    }

    // 0x1E0
    float* pSpeedMult = reinterpret_cast<float*>(pPlayer + Offsets::PlayerVelocityX);
    if (!IsBadReadPtr(pSpeedMult, sizeof(float))) {
        const float targetSpeed = 1.0f + (vars.movementSpeedBonus * 10.0f);
        if (*pSpeedMult >= 0.0f && *pSpeedMult <= 100.0f) {
            *pSpeedMult = targetSpeed;
        }
    }
}

}  // namespace got
