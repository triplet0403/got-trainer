#pragma once

#include <cstdint>

namespace got {

// Lightweight in-game toast notification system used to give the user
// visual feedback when a feature is toggled or an action completes,
// without needing to open the log file.
enum class ToastLevel : uint8_t {
    Info = 0,
    Success,
    Warning,
    Error,
};

// Queues a toast message to be drawn for a few seconds in the top-right
// corner of the screen. Safe to call from any feature module.
void PushToast(ToastLevel level, const char* fmt, ...);

// Internal: used only by the UI layer to draw + age out active toasts.
void DrawToasts();

}  // namespace got
