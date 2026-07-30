#include "ui/trainer_ui.hpp"

#include <Windows.h>
#include <imgui.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "core/config.hpp"
#include "core/game_pointers.hpp"
#include "core/notify.hpp"
#include "core/offsets.hpp"
#include "core/trainer_state.hpp"
#include "features/item_features.hpp"

namespace got {

namespace {

//======================================================================
// Theme
//======================================================================
// Accent: warm gold (nods to Jin's sword / the in-game UI) on a near-black
// slate background, with soft rounding throughout so it reads as a
// deliberately designed overlay rather than a stock ImGui demo window.
constexpr ImVec4 kAccent      = ImVec4(0.85f, 0.65f, 0.25f, 1.00f);
constexpr ImVec4 kAccentHover = ImVec4(0.95f, 0.75f, 0.35f, 1.00f);
constexpr ImVec4 kAccentActive= ImVec4(0.75f, 0.55f, 0.18f, 1.00f);
constexpr ImVec4 kBgWindow    = ImVec4(0.07f, 0.075f, 0.085f, 0.97f);
constexpr ImVec4 kBgChild     = ImVec4(0.10f, 0.105f, 0.12f, 1.00f);
constexpr ImVec4 kGood        = ImVec4(0.35f, 0.85f, 0.45f, 1.00f);
constexpr ImVec4 kBad         = ImVec4(0.90f, 0.35f, 0.35f, 1.00f);
constexpr ImVec4 kMuted       = ImVec4(0.55f, 0.55f, 0.58f, 1.00f);

struct ResourceEntry {
    uint32_t id;
    const char* name;
};

constexpr ResourceEntry kResources[] = {
    {0x00, "Arrow"},
    {0x13, "Bamboo"},
    {0x18, "Bear Hide"},
    {0x0D, "Blackpowder Bomb"},
    {0x19, "Boar Hide"},
    {0x1A, "Bones"},
    {0x1B, "Cloth"},
    {0x1C, "Deer Hide"},
    {0x04, "Explosive Arrow"},
    {0x1D, "Feather"},
    {0x0B, "Firecracker"},
    {0x2C, "Flowers"},
    {0x01, "Flaming Arrow"},
    {0x2F, "Ghost Flowers"},
    {0x1E, "Gold"},
    {0x20, "Green Herb"},
    {0x07, "Hallucination Dart"},
    {0x02, "Heavy Arrow"},
    {0x16, "Iron"},
    {0x12, "Incendiary Oil"},
    {0x0F, "Kunai"},
    {0x25, "Linen"},
    {0x26, "Leather"},
    {0x2B, "Predator Hides"},
    {0x09, "Poison Dart"},
    {0x10, "Smoke Bomb"},
    {0x11, "Sticky Bomb"},
    {0x22, "Silver"},
    {0x27, "Silk"},
    {0x21, "Sinew"},
    {0x1F, "Steel"},
    {0x2A, "Supplies"},
    {0x23, "Talon"},
    {0x17, "Toxin"},
    {0x24, "Venom Pouch"},
    {0x0C, "Windchime"},
    {0x29, "Wax Wood"},
    {0x28, "Yew Wood"},
};
constexpr int kResourceCount = sizeof(kResources) / sizeof(kResources[0]);

//======================================================================
// Small reusable widgets
//======================================================================

void SectionHeader(const char* label) {
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, kAccent);
    ImGui::TextUnformatted(label);
    ImGui::PopStyleColor();
    ImGui::Separator();
    ImGui::Spacing();
}

// A checkbox styled as a small "pill" toggle switch for the primary
// feature list, so the busiest tabs (Player/Combat) read as a clean
// on/off panel instead of a wall of native checkboxes.
bool ToggleSwitch(const char* label, bool* value) {
    ImGui::PushID(label);
    bool changed = ImGui::Checkbox("##toggle", value);
    ImGui::SameLine();
    ImGui::TextUnformatted(label);
    ImGui::PopID();
    return changed;
}

const char* KeyName(int vk) {
    static char buf[32];
    if (vk <= 0) {
        return "Unbound";
    }
    UINT scan = MapVirtualKeyA(static_cast<UINT>(vk), MAPVK_VK_TO_VSC);
    LONG lparam = static_cast<LONG>(scan) << 16;
    // Extended-key bit for keys like arrows / navigation cluster / numpad enter.
    switch (vk) {
        case VK_LEFT: case VK_RIGHT: case VK_UP: case VK_DOWN:
        case VK_HOME: case VK_END: case VK_PRIOR: case VK_NEXT:
        case VK_INSERT: case VK_DELETE:
            lparam |= (1 << 24);
            break;
        default: break;
    }
    if (GetKeyNameTextA(lparam, buf, sizeof(buf)) == 0) {
        snprintf(buf, sizeof(buf), "VK 0x%02X", vk);
    }
    return buf;
}

// Scans VK codes for any key currently held down; returns 0 if none.
// Used for the "click to rebind" flow below.
int PollAnyKeyDown() {
    for (int vk = 0x08; vk < 0xFE; ++vk) {
        if (vk == VK_LBUTTON || vk == VK_RBUTTON) continue;  // don't eat mouse clicks used to open the rebind box
        if (GetAsyncKeyState(vk) & 0x8000) {
            return vk;
        }
    }
    return 0;
}

// Renders a "[ Key Name ][Rebind]" row. While listening, shows a pulsing
// prompt and captures the next key press.
void HotkeyRow(const char* label, int* vkSlot) {
    static int listeningFor = -1;  // pointer identity via index trick below
    ImGui::PushID(label);

    ImGui::TextUnformatted(label);
    ImGui::SameLine(180);

    bool isListening = (listeningFor == reinterpret_cast<intptr_t>(vkSlot));
    if (isListening) {
        ImGui::PushStyleColor(ImGuiCol_Button, kAccentActive);
        ImGui::Button("Press any key...", ImVec2(150, 0));
        ImGui::PopStyleColor();
        int pressed = PollAnyKeyDown();
        if (pressed == VK_ESCAPE) {
            listeningFor = -1;  // cancel
        } else if (pressed != 0) {
            *vkSlot = pressed;
            listeningFor = -1;
        }
    } else {
        char btnLabel[48];
        snprintf(btnLabel, sizeof(btnLabel), "%s##bind", KeyName(*vkSlot));
        if (ImGui::Button(btnLabel, ImVec2(150, 0))) {
            listeningFor = reinterpret_cast<intptr_t>(vkSlot);
        }
    }

    ImGui::PopID();
}

//======================================================================
// Tabs
//======================================================================

void DrawHomeTab() {
    const auto& settings = GetTrainerSettings();
    const auto& ptrs = GetGamePointers();

    ImGui::Spacing();
    ImGui::TextColored(kAccent, "Ghost of Tsushima Trainer");
    ImGui::TextColored(kMuted, "Game version: %s", kGameVersion);
    ImGui::Spacing();

    const bool ready = ptrs.allRootScanned();
    ImGui::TextUnformatted("Status:");
    ImGui::SameLine();
    if (ready) {
        ImGui::TextColored(kGood, "Ready");
    } else {
        ImGui::TextColored(ImVec4(1.0f, 0.65f, 0.25f, 1.0f), "Partially loaded (open Status tab)");
    }

    SectionHeader("Active Features");

    int activeCount = 0;
    auto listIf = [&](bool on, const char* name) {
        if (!on) return;
        ++activeCount;
        ImGui::BulletText("%s", name);
    };
    listIf(settings.infiniteHealth, "Infinite Health");
    listIf(settings.infiniteResolve, "Infinite Resolve");
    listIf(settings.stealth, "Stealth");
    listIf(settings.noIndicator, "No Detection Indicator");
    listIf(settings.forcePerfectDodge, "Force Perfect Dodge");
    listIf(settings.autoBlockMelee, "Auto Block Melee");
    listIf(settings.alwaysParryOnBlock, "Always Parry on Block");
    listIf(settings.perfectParry, "Perfect Parry");
    listIf(settings.movementSpeed, "Movement Speed Hack");
    listIf(settings.freeCam, "Free Camera");
    if (activeCount == 0) {
        ImGui::TextColored(kMuted, "Nothing enabled yet. Head to the Player, Combat, Movement or Camera tabs.");
    }

    SectionHeader("Quick Reference");
    ImGui::BulletText("%s - Open / close this menu", KeyName(GetTrainerHotkeys().menuToggle));
    ImGui::BulletText("F1 / F2 - Save / teleport to saved position");
    ImGui::BulletText("F3 - Teleport player to free-cam position");
    ImGui::BulletText("%s - Panic switch (disable everything instantly)", KeyName(GetTrainerHotkeys().panic));
    ImGui::TextColored(kMuted, "Customize hotkeys in the Hotkeys tab.");
}

void DrawStatusTab() {
    auto& ptrs = GetGamePointers();
    const auto* results = ptrs.GetResults();
    const size_t count = ptrs.ResultCount();

    ImGui::TextColored(kMuted, "Internal component health check. No memory addresses are shown here.");
    ImGui::Spacing();

    size_t okCount = 0;
    for (size_t i = 0; i < count; ++i) {
        if (results[i].ok) ++okCount;
        ImGui::PushStyleColor(ImGuiCol_Text, results[i].ok ? kGood : kBad);
        ImGui::Text("  %s  %s", results[i].ok ? "[OK]" : "[--]", results[i].name.c_str());
        ImGui::PopStyleColor();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::Text("%zu / %zu components ready", okCount, count);

    ImGui::Spacing();
    if (ImGui::Button("Rescan", ImVec2(160, 30))) {
        ptrs.Rescan();
        PushToast(ToastLevel::Info, "Rescanned game memory");
    }
    ImGui::SameLine();
    if (ptrs.allRootScanned()) {
        ImGui::TextColored(kGood, "All systems ready");
    } else {
        ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), "Some features unavailable until fully loaded into a save");
    }
}

void DrawPlayerTab() {
    auto& settings = GetTrainerSettings();
    auto& actions = GetTrainerActions();
    auto& ptrs = GetGamePointers();

    SectionHeader("Survivability");
    ToggleSwitch("Infinite Health", &settings.infiniteHealth);
    ToggleSwitch("Infinite Resolve", &settings.infiniteResolve);

    ImGui::Spacing();
    if (ImGui::Button("Fill Health", ImVec2(140, 28))) actions.fillHealth = true;
    ImGui::SameLine();
    if (ImGui::Button("Fill Resolve", ImVec2(140, 28))) actions.fillResolve = true;

    SectionHeader("Live Values");
    if (ptrs.playerInstance) {
        uintptr_t player = *reinterpret_cast<uintptr_t*>(ptrs.playerInstance);
        if (player) {
            float health = *reinterpret_cast<float*>(player + Offsets::PlayerHealth);
            float maxHealth = *reinterpret_cast<float*>(player + Offsets::PlayerMaxHealth);
            float resolve = *reinterpret_cast<float*>(player + Offsets::PlayerResolve);

            ImGui::Text("Health:  %.0f / %.0f", health, maxHealth);
            float healthPct = (maxHealth > 0) ? (health / maxHealth) : 0.0f;
            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, kAccent);
            ImGui::ProgressBar(healthPct, ImVec2(-1, 18), "");
            ImGui::PopStyleColor();

            ImGui::Spacing();
            ImGui::Text("Resolve: %.2f / 4.00", resolve);
        } else {
            ImGui::TextColored(ImVec4(1, 0.6f, 0.3f, 1), "Player not loaded - enter gameplay to see live values");
        }
    } else {
        ImGui::TextColored(kBad, "Player data unavailable (see Status tab)");
    }

    if (ptrs.gearInstance) {
        int32_t techPoints = *reinterpret_cast<int32_t*>(ptrs.gearInstance + 2 * 4 + Offsets::GearTechniquePointsIndex);
        ImGui::Spacing();
        ImGui::Text("Technique Points: %d", techPoints);
    }
}

void DrawCombatTab() {
    auto& settings = GetTrainerSettings();

    SectionHeader("Detection");
    ToggleSwitch("Stealth (Invisible to enemies)", &settings.stealth);
    ToggleSwitch("No Detection Indicator", &settings.noIndicator);

    SectionHeader("Defense");
    ToggleSwitch("Force Perfect Dodge", &settings.forcePerfectDodge);
    ToggleSwitch("Auto Block Melee", &settings.autoBlockMelee);
    ToggleSwitch("Always Parry on Block", &settings.alwaysParryOnBlock);
    ToggleSwitch("Perfect Parry", &settings.perfectParry);

    if (settings.alwaysParryOnBlock && settings.perfectParry) {
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.3f, 1.0f), "Note: both parry options patch the same instruction; either one alone is enough.");
    }
}

void DrawMovementTab() {
    auto& settings = GetTrainerSettings();
    auto& vars = GetTrainerVars();

    SectionHeader("Speed");
    ToggleSwitch("Movement Speed Hack", &settings.movementSpeed);
    ImGui::SliderFloat("Speed Bonus", &vars.movementSpeedBonus, 0.0f, 1.0f, "%.3f");

    SectionHeader("Pickups");
    ImGui::BeginDisabled();
    ToggleSwitch("Pickup Multiplier (in progress)", &settings.pickupMultiplier);
    int pickupMul = static_cast<int>(vars.pickupMultiplier);
    ImGui::SliderInt("Multiplier", &pickupMul, 1, 100);
    ImGui::EndDisabled();
    ImGui::TextColored(kMuted,
        "Needs a fresh signature for the pickup call site after the last game update.\n"
        "Left visible so the setting isn't lost - will be enabled in a follow-up build.");
}

void DrawItemsTab() {
    auto& settings = GetTrainerSettings();
    auto& vars = GetTrainerVars();
    auto& ptrs = GetGamePointers();

    SectionHeader("Add Resource");
    static int selectedResource = 0;
    if (ImGui::BeginCombo("Resource", kResources[selectedResource].name)) {
        for (int i = 0; i < kResourceCount; ++i) {
            bool isSelected = (selectedResource == i);
            if (ImGui::Selectable(kResources[i].name, isSelected)) {
                selectedResource = i;
                vars.resourceId = kResources[i].id;
            }
            if (isSelected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    int amount = static_cast<int>(vars.resourceAmount);
    if (ImGui::InputInt("Amount", &amount, 10, 100)) {
        if (amount < 0) amount = 0;
        vars.resourceAmount = static_cast<uint32_t>(amount);
    }

    bool canAddResource = ptrs.addResourceFunction && ptrs.inventoryInstance;
    if (!canAddResource) ImGui::BeginDisabled();
    if (ImGui::Button("Add Resource", ImVec2(200, 30))) {
        AddResourceToInventory(vars.resourceId, vars.resourceAmount);
    }
    if (!canAddResource) {
        ImGui::EndDisabled();
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
            ImGui::SetTooltip("Waiting on a full pointer scan - check the Status tab.");
        }
    }

    SectionHeader("Give Gear");
    char hashBuf[32];
    snprintf(hashBuf, sizeof(hashBuf), "%016llX", static_cast<unsigned long long>(vars.gearHash));
    if (ImGui::InputText("Gear Hash", hashBuf, sizeof(hashBuf), ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase)) {
        vars.gearHash = strtoull(hashBuf, nullptr, 16);
    }

    bool canGiveGear = ptrs.addGearFunction && ptrs.gearInstance && ptrs.templateList && ptrs.permGear;
    if (!canGiveGear) ImGui::BeginDisabled();
    if (ImGui::Button("Give Gear", ImVec2(200, 30))) {
        GiveGear(vars.gearHash);
    }
    if (!canGiveGear) {
        ImGui::EndDisabled();
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
            ImGui::SetTooltip("Waiting on a full pointer scan - check the Status tab.");
        }
    }

    SectionHeader("Save Compatibility");
    ImGui::BeginDisabled();
    ToggleSwitch("Unlock All Gear on Save Load (in progress)", &settings.gearUnlockOnLoad);
    ImGui::EndDisabled();
    ImGui::TextColored(kMuted, "Requires enumerating the gear template list at load time - tracked for a future update.");
}

void DrawCameraTab() {
    auto& settings = GetTrainerSettings();
    auto& vars = GetTrainerVars();

    SectionHeader("Free Camera");
    ToggleSwitch("Free Camera", &settings.freeCam);
    ImGui::SliderFloat("Cam Speed", &vars.freeCamSpeed, 1.0f, 100.0f, "%.1f");

    SectionHeader("Controls");
    ImGui::BulletText("W/S or I/K - Forward / Backward");
    ImGui::BulletText("Q/E or U/O - Up / Down");
    ImGui::BulletText("J/L - Left / Right");
    ImGui::BulletText("Shift - Speed boost (x3)");
    ImGui::BulletText("F3 - Teleport player to camera");
}

void DrawHotkeysTab() {
    auto& hk = GetTrainerHotkeys();

    ImGui::TextColored(kMuted, "Click a key, then press any key to rebind. Esc cancels.");
    SectionHeader("Global Hotkeys");
    HotkeyRow("Open / Close Menu", &hk.menuToggle);
    HotkeyRow("Toggle Infinite Health", &hk.infiniteHealth);
    HotkeyRow("Toggle Stealth", &hk.stealth);
    HotkeyRow("Toggle Free Camera", &hk.freeCam);
    HotkeyRow("Panic (disable everything)", &hk.panic);

    SectionHeader("Fixed Keys");
    ImGui::TextColored(kMuted, "These stay fixed to avoid clashing with the rebindable set above.");
    ImGui::BulletText("F1 - Save current position");
    ImGui::BulletText("F2 - Teleport to saved position");
    ImGui::BulletText("F3 - Teleport player to free-cam position");
}

void DrawSettingsTab() {
    auto& settings = GetTrainerSettings();

    SectionHeader("General");
    ImGui::Checkbox("Auto-save settings on exit", &settings.autoSaveConfig);
    ImGui::Checkbox("Show on-screen notifications", &settings.showToasts);

    SectionHeader("Profile");
    if (ImGui::Button("Save Settings Now", ImVec2(180, 30))) {
        SaveConfig();
        PushToast(ToastLevel::Success, "Settings saved");
    }
    ImGui::SameLine();
    if (ImGui::Button("Reload From Disk", ImVec2(180, 30))) {
        LoadConfig();
        PushToast(ToastLevel::Info, "Settings reloaded");
    }
    ImGui::TextColored(kMuted, "Stored in got_trainer.ini next to the game executable.");

    SectionHeader("Safety");
    bool master = settings.masterEnable;
    ImGui::PushStyleColor(ImGuiCol_Text, master ? kGood : kBad);
    ImGui::Text(master ? "Trainer is ENABLED" : "Trainer is DISABLED (panic mode)");
    ImGui::PopStyleColor();
    if (ImGui::Button(master ? "Disable Everything" : "Re-enable Trainer", ImVec2(220, 30))) {
        settings.masterEnable = !settings.masterEnable;
        PushToast(settings.masterEnable ? ToastLevel::Success : ToastLevel::Warning,
                   settings.masterEnable ? "Trainer enabled" : "Trainer disabled");
    }
    ImGui::TextColored(kMuted, "Same as pressing the Panic hotkey. All memory patches are restored instantly.");
}

}  // namespace

//======================================================================
// Theme application
//======================================================================
void ApplyTrainerTheme() {
    ImGuiStyle& style = ImGui::GetStyle();

    style.WindowRounding    = 8.0f;
    style.ChildRounding     = 6.0f;
    style.FrameRounding     = 4.0f;
    style.PopupRounding     = 6.0f;
    style.ScrollbarRounding = 6.0f;
    style.GrabRounding      = 4.0f;
    style.TabRounding       = 5.0f;
    style.WindowPadding     = ImVec2(14, 14);
    style.FramePadding      = ImVec2(8, 5);
    style.ItemSpacing       = ImVec2(8, 8);
    style.WindowBorderSize  = 1.0f;
    style.FrameBorderSize   = 0.0f;
    style.GrabMinSize       = 10.0f;

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg]        = kBgWindow;
    colors[ImGuiCol_ChildBg]         = kBgChild;
    colors[ImGuiCol_PopupBg]         = ImVec4(0.09f, 0.09f, 0.10f, 0.98f);
    colors[ImGuiCol_Border]          = ImVec4(0.30f, 0.24f, 0.14f, 0.55f);
    colors[ImGuiCol_FrameBg]         = ImVec4(0.16f, 0.16f, 0.18f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]  = ImVec4(0.22f, 0.20f, 0.16f, 1.00f);
    colors[ImGuiCol_FrameBgActive]   = ImVec4(0.26f, 0.22f, 0.14f, 1.00f);
    colors[ImGuiCol_TitleBg]         = ImVec4(0.08f, 0.08f, 0.09f, 1.00f);
    colors[ImGuiCol_TitleBgActive]   = ImVec4(0.12f, 0.10f, 0.07f, 1.00f);
    colors[ImGuiCol_CheckMark]       = kAccent;
    colors[ImGuiCol_SliderGrab]      = kAccent;
    colors[ImGuiCol_SliderGrabActive]= kAccentActive;
    colors[ImGuiCol_Button]          = ImVec4(0.20f, 0.17f, 0.11f, 1.00f);
    colors[ImGuiCol_ButtonHovered]   = ImVec4(kAccentHover.x, kAccentHover.y, kAccentHover.z, 0.35f);
    colors[ImGuiCol_ButtonActive]    = ImVec4(kAccentActive.x, kAccentActive.y, kAccentActive.z, 0.55f);
    colors[ImGuiCol_Header]          = ImVec4(0.22f, 0.18f, 0.11f, 1.00f);
    colors[ImGuiCol_HeaderHovered]   = ImVec4(0.30f, 0.24f, 0.14f, 1.00f);
    colors[ImGuiCol_HeaderActive]    = ImVec4(0.34f, 0.27f, 0.15f, 1.00f);
    colors[ImGuiCol_Tab]             = ImVec4(0.12f, 0.12f, 0.13f, 1.00f);
    colors[ImGuiCol_TabHovered]      = ImVec4(kAccentHover.x, kAccentHover.y, kAccentHover.z, 0.55f);
    colors[ImGuiCol_TabActive]       = ImVec4(0.24f, 0.19f, 0.10f, 1.00f);
    colors[ImGuiCol_TabUnfocused]    = ImVec4(0.10f, 0.10f, 0.11f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.16f, 0.14f, 0.09f, 1.00f);
    colors[ImGuiCol_Separator]       = ImVec4(0.30f, 0.24f, 0.14f, 0.45f);
    colors[ImGuiCol_PlotHistogram]   = kAccent;
    colors[ImGuiCol_ScrollbarGrab]   = ImVec4(0.28f, 0.22f, 0.13f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(kAccentHover.x, kAccentHover.y, kAccentHover.z, 0.6f);
    colors[ImGuiCol_ScrollbarGrabActive]  = kAccentActive;
    colors[ImGuiCol_Text]            = ImVec4(0.92f, 0.92f, 0.94f, 1.00f);
    colors[ImGuiCol_TextDisabled]    = kMuted;
}

//======================================================================
// Main render entry point
//======================================================================
void RenderTrainerUI() {
    auto& settings = GetTrainerSettings();

    ImGui::SetNextWindowSize(ImVec2(560, 520), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(50, 50), ImGuiCond_FirstUseEver);

    ImGui::Begin("Ghost of Tsushima Trainer", nullptr, ImGuiWindowFlags_NoCollapse);

    // Always-visible master switch at the top of the window - the fastest
    // way to kill every active effect before tabbing out or recording.
    ImGui::PushStyleColor(ImGuiCol_Button,
        settings.masterEnable ? ImVec4(0.15f, 0.35f, 0.18f, 1.0f) : ImVec4(0.40f, 0.14f, 0.14f, 1.0f));
    if (ImGui::Button(settings.masterEnable ? "TRAINER: ON" : "TRAINER: OFF", ImVec2(140, 30))) {
        settings.masterEnable = !settings.masterEnable;
    }
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::TextColored(kMuted, "(%s to toggle from anywhere)", KeyName(GetTrainerHotkeys().panic));

    ImGui::Spacing();

    if (!settings.masterEnable) {
        ImGui::BeginDisabled();
    }

    if (ImGui::BeginTabBar("TrainerTabs")) {
        if (ImGui::BeginTabItem("Home"))     { DrawHomeTab();     ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem("Player"))   { DrawPlayerTab();   ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem("Combat"))   { DrawCombatTab();   ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem("Movement")) { DrawMovementTab(); ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem("Camera"))   { DrawCameraTab();   ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem("Items"))    { DrawItemsTab();    ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem("Hotkeys"))  { DrawHotkeysTab();  ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem("Settings")) { DrawSettingsTab(); ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem("Status"))   { DrawStatusTab();   ImGui::EndTabItem(); }
        ImGui::EndTabBar();
    }

    if (!settings.masterEnable) {
        ImGui::EndDisabled();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextColored(kMuted, "Press %s to toggle this menu", KeyName(GetTrainerHotkeys().menuToggle));

    ImGui::End();
}

}  // namespace got
