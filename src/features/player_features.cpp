#include "features/player_features.hpp"

#include <Windows.h>

#include "core/game_pointers.hpp"
#include "core/log.hpp"
#include "core/notify.hpp"
#include "core/offsets.hpp"
#include "core/trainer_state.hpp"

namespace got {

namespace {
float g_savedPos[3] = {0.0f, 0.0f, 0.0f};
bool g_hasSavedPos = false;
}  // namespace

void InitPlayerFeatures() {
    Log("[Player] Initialized Player features module.");
}

void ShutdownPlayerFeatures() {
    g_hasSavedPos = false;
}

void UpdatePlayerFeatures() {
    const auto& settings = GetTrainerSettings();
    auto& actions = GetTrainerActions();

    if (!settings.masterEnable) {
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

    float* pHealth = reinterpret_cast<float*>(pPlayer + Offsets::PlayerHealth);
    float* pMaxHealth = reinterpret_cast<float*>(pPlayer + Offsets::PlayerMaxHealth);
    float* pResolve = reinterpret_cast<float*>(pPlayer + Offsets::PlayerResolve);

    // 1. Infinite Health
    if (settings.infiniteHealth) {
        if (!IsBadReadPtr(pMaxHealth, sizeof(float)) && *pMaxHealth > 0.0f) {
            *pHealth = *pMaxHealth;
        } else if (!IsBadReadPtr(pHealth, sizeof(float))) {
            *pHealth = 1000.0f;
        }
    }

    //    vice versa).
    if (settings.infiniteResolve) {
        if (!IsBadReadPtr(pResolve, sizeof(float))) {
            *pResolve = 4.0f;  // max resolve is 4 stacks in the base game
        }
    }

    //    here .
    if (actions.fillHealth) {
        if (!IsBadReadPtr(pMaxHealth, sizeof(float)) && !IsBadReadPtr(pHealth, sizeof(float)) && *pMaxHealth > 0.0f) {
            *pHealth = *pMaxHealth;
            PushToast(ToastLevel::Success, "Health topped up");
        }
        actions.fillHealth = false;
    }
    if (actions.fillResolve) {
        if (!IsBadReadPtr(pResolve, sizeof(float))) {
            *pResolve = 4.0f;
            PushToast(ToastLevel::Success, "Resolve topped up");
        }
        actions.fillResolve = false;
    }


    // F1 = Save Position
    if (GetAsyncKeyState(VK_F1) & 1) {
        float* pPos = reinterpret_cast<float*>(pPlayer + Offsets::PlayerPositionW);
        if (!IsBadReadPtr(pPos, sizeof(float) * 3)) {
            g_savedPos[0] = pPos[0];
            g_savedPos[1] = pPos[1];
            g_savedPos[2] = pPos[2];
            g_hasSavedPos = true;
            Log("[Player] Saved position: (%.2f, %.2f, %.2f)", g_savedPos[0], g_savedPos[1], g_savedPos[2]);
            PushToast(ToastLevel::Info, "Position saved");
        }
    }

    // F2 = Load Saved Position
    if (GetAsyncKeyState(VK_F2) & 1) {
        if (g_hasSavedPos) {
            float* pPos = reinterpret_cast<float*>(pPlayer + Offsets::PlayerPositionW);
            if (!IsBadReadPtr(pPos, sizeof(float) * 3)) {
                pPos[0] = g_savedPos[0];
                pPos[1] = g_savedPos[1];
                pPos[2] = g_savedPos[2];
                Log("[Player] Teleported to saved position: (%.2f, %.2f, %.2f)", g_savedPos[0], g_savedPos[1], g_savedPos[2]);
                PushToast(ToastLevel::Success, "Teleported to saved position");
            }
        } else {
            PushToast(ToastLevel::Warning, "No saved position (press F1 first)");
        }
    }
}

}  // namespace got
