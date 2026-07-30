#include "core/notify.hpp"

#include <Windows.h>
#include <imgui.h>
#include <cstdarg>
#include <cstdio>
#include <array>
#include <mutex>

namespace got {

namespace {

struct Toast {
    bool active = false;
    ToastLevel level = ToastLevel::Info;
    char text[192] = {};
    float remaining = 0.0f;  // seconds
};

constexpr int kMaxToasts = 5;
constexpr float kToastLifetime = 3.0f;

std::array<Toast, kMaxToasts> g_toasts;
std::mutex g_toastMutex;

ImVec4 ColorForLevel(ToastLevel level) {
    switch (level) {
        case ToastLevel::Success: return ImVec4(0.35f, 0.90f, 0.55f, 1.0f);
        case ToastLevel::Warning: return ImVec4(1.00f, 0.75f, 0.25f, 1.0f);
        case ToastLevel::Error:   return ImVec4(1.00f, 0.35f, 0.35f, 1.0f);
        default:                  return ImVec4(0.45f, 0.75f, 1.00f, 1.0f);
    }
}

}  // namespace

void PushToast(ToastLevel level, const char* fmt, ...) {
    char buffer[192];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    std::lock_guard<std::mutex> lock(g_toastMutex);
    // Find a free slot, or steal the oldest (smallest remaining time) one.
    int slot = -1;
    float smallest = 1e9f;
    for (int i = 0; i < kMaxToasts; ++i) {
        if (!g_toasts[i].active) {
            slot = i;
            break;
        }
        if (g_toasts[i].remaining < smallest) {
            smallest = g_toasts[i].remaining;
            slot = i;
        }
    }
    if (slot < 0) return;

    g_toasts[slot].active = true;
    g_toasts[slot].level = level;
    g_toasts[slot].remaining = kToastLifetime;
    snprintf(g_toasts[slot].text, sizeof(g_toasts[slot].text), "%s", buffer);
}

void DrawToasts() {
    std::lock_guard<std::mutex> lock(g_toastMutex);

    const ImGuiIO& io = ImGui::GetIO();
    const float dt = io.DeltaTime > 0.0f ? io.DeltaTime : (1.0f / 60.0f);

    float yOffset = 20.0f;
    const float margin = 16.0f;

    for (auto& toast : g_toasts) {
        if (!toast.active) continue;

        toast.remaining -= dt;
        if (toast.remaining <= 0.0f) {
            toast.active = false;
            continue;
        }

        float alpha = 1.0f;
        if (toast.remaining < 0.5f) alpha = toast.remaining / 0.5f;
        if (toast.remaining > kToastLifetime - 0.25f) alpha = (kToastLifetime - toast.remaining) / 0.25f;

        ImVec2 textSize = ImGui::CalcTextSize(toast.text);
        float width = textSize.x + 40.0f;
        float height = 34.0f;

        ImVec2 viewportSize = io.DisplaySize;
        ImVec2 pos(viewportSize.x - width - margin, yOffset);

        ImDrawList* drawList = ImGui::GetForegroundDrawList();
        ImVec4 accent = ColorForLevel(toast.level);
        accent.w = alpha;

        ImU32 bg = ImGui::ColorConvertFloat4ToU32(ImVec4(0.08f, 0.08f, 0.10f, 0.92f * alpha));
        ImU32 border = ImGui::ColorConvertFloat4ToU32(accent);
        ImU32 text = ImGui::ColorConvertFloat4ToU32(ImVec4(0.95f, 0.95f, 0.97f, alpha));

        drawList->AddRectFilled(pos, ImVec2(pos.x + width, pos.y + height), bg, 6.0f);
        drawList->AddRectFilled(pos, ImVec2(pos.x + 4.0f, pos.y + height), border, 6.0f);
        drawList->AddRect(pos, ImVec2(pos.x + width, pos.y + height), border, 6.0f, 0, 1.5f);
        drawList->AddText(ImVec2(pos.x + 16.0f, pos.y + (height - textSize.y) * 0.5f), text, toast.text);

        yOffset += height + 8.0f;
    }
}

}  // namespace got
