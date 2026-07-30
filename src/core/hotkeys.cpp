#include "core/hotkeys.hpp"

#include <Windows.h>

#include "core/notify.hpp"
#include "core/trainer_state.hpp"

namespace got {

namespace {

bool Pressed(int vk) {
    if (vk <= 0) return false;
    return (GetAsyncKeyState(vk) & 1) != 0;  // edge-triggered (key-down since last poll)
}

}  // namespace

void ProcessHotkeys() {
    auto& settings = GetTrainerSettings();
    const auto& hotkeys = GetTrainerHotkeys();

    if (Pressed(hotkeys.panic)) {
        settings.masterEnable = !settings.masterEnable;
        PushToast(settings.masterEnable ? ToastLevel::Success : ToastLevel::Warning,
                   settings.masterEnable ? "Trainer ENABLED" : "Trainer DISABLED (panic)");
    }

    if (!settings.masterEnable) {
        return;  // ignore feature hotkeys while panicked-off
    }

    if (Pressed(hotkeys.infiniteHealth)) {
        settings.infiniteHealth = !settings.infiniteHealth;
        PushToast(settings.infiniteHealth ? ToastLevel::Success : ToastLevel::Info,
                   "Infinite Health: %s", settings.infiniteHealth ? "ON" : "OFF");
    }

    if (Pressed(hotkeys.stealth)) {
        settings.stealth = !settings.stealth;
        PushToast(settings.stealth ? ToastLevel::Success : ToastLevel::Info,
                   "Stealth: %s", settings.stealth ? "ON" : "OFF");
    }

    if (Pressed(hotkeys.freeCam)) {
        settings.freeCam = !settings.freeCam;
        PushToast(settings.freeCam ? ToastLevel::Success : ToastLevel::Info,
                   "Free Camera: %s", settings.freeCam ? "ON" : "OFF");
    }
}

}  // namespace got
