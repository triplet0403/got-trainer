#pragma once

#include <cstdint>

namespace got {

enum class ToastLevel : uint8_t {
    Info = 0,
    Success,
    Warning,
    Error,
};

void PushToast(ToastLevel level, const char* fmt, ...);

void DrawToasts();

}  // namespace got
