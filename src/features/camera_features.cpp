#include "features/camera_features.hpp"

#include <Windows.h>

#include "core/game_pointers.hpp"
#include "core/log.hpp"
#include "core/notify.hpp"
#include "core/offsets.hpp"
#include "core/trainer_state.hpp"

namespace got {

void InitCameraFeatures() {
    Log("[Camera] Initialized Camera feature module.");
}

void ShutdownCameraFeatures() {
}

void UpdateCameraFeatures() {
    const auto& settings = GetTrainerSettings();
    const auto& vars = GetTrainerVars();

    if (!settings.masterEnable || !settings.freeCam) {
        return;
    }

    // Try camera global pointer or fallback to player position offset for camera
    const uintptr_t pCamGlobal = GetGamePointers().camRotGlobal;
    uintptr_t pCam = 0;
    if (pCamGlobal && !IsBadReadPtr(reinterpret_cast<void*>(pCamGlobal), sizeof(uintptr_t))) {
        pCam = *reinterpret_cast<uintptr_t*>(pCamGlobal);
    }

    if (!pCam) {
        const uintptr_t pPlayerGlobal = GetGamePointers().playerInstance;
        if (pPlayerGlobal && !IsBadReadPtr(reinterpret_cast<void*>(pPlayerGlobal), sizeof(uintptr_t))) {
            pCam = *reinterpret_cast<uintptr_t*>(pPlayerGlobal);
        }
    }

    if (!pCam || IsBadReadPtr(reinterpret_cast<void*>(pCam), 0x2000)) {
        return;
    }

    float* pCamPos = reinterpret_cast<float*>(pCam + Offsets::CamPosition);
    if (IsBadReadPtr(pCamPos, sizeof(float) * 3)) {
        pCamPos = reinterpret_cast<float*>(pCam + Offsets::PlayerPositionW);
        if (IsBadReadPtr(pCamPos, sizeof(float) * 3)) {
            return;
        }
    }

    float speed = vars.freeCamSpeed * 0.1f;
    if (GetAsyncKeyState(VK_SHIFT) & 0x8000) {
        speed *= 3.0f;
    }


    if ((GetAsyncKeyState('I') & 0x8000) || (GetAsyncKeyState(VK_NUMPAD8) & 0x8000)) pCamPos[1] += speed;
    if ((GetAsyncKeyState('K') & 0x8000) || (GetAsyncKeyState(VK_NUMPAD2) & 0x8000)) pCamPos[1] -= speed;
    if ((GetAsyncKeyState('J') & 0x8000) || (GetAsyncKeyState(VK_NUMPAD4) & 0x8000)) pCamPos[0] -= speed;
    if ((GetAsyncKeyState('L') & 0x8000) || (GetAsyncKeyState(VK_NUMPAD6) & 0x8000)) pCamPos[0] += speed;
    if ((GetAsyncKeyState('U') & 0x8000) || (GetAsyncKeyState(VK_NUMPAD9) & 0x8000)) pCamPos[2] += speed;
    if ((GetAsyncKeyState('O') & 0x8000) || (GetAsyncKeyState(VK_NUMPAD3) & 0x8000)) pCamPos[2] -= speed;


    if (GetAsyncKeyState(VK_F3) & 1) {
        const uintptr_t pPlayerGlobal = GetGamePointers().playerInstance;
        if (pPlayerGlobal && !IsBadReadPtr(reinterpret_cast<void*>(pPlayerGlobal), sizeof(uintptr_t))) {
            const uintptr_t pPlayer = *reinterpret_cast<uintptr_t*>(pPlayerGlobal);
            if (pPlayer && !IsBadReadPtr(reinterpret_cast<void*>(pPlayer), 0x2000)) {
                float* pPlayerPos = reinterpret_cast<float*>(pPlayer + Offsets::PlayerPositionW);
                if (!IsBadReadPtr(pPlayerPos, sizeof(float) * 3)) {
                    pPlayerPos[0] = pCamPos[0];
                    pPlayerPos[1] = pCamPos[1];
                    pPlayerPos[2] = pCamPos[2];
                    Log("[Camera] Teleported player to free-cam position.");
                    PushToast(ToastLevel::Success, "Teleported to camera");
                }
            }
        }
    }
}

}  // namespace got
