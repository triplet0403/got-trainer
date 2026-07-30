#include "core/config.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "core/log.hpp"
#include "core/trainer_state.hpp"

namespace got {

namespace {
constexpr const char* kConfigFileName = "got_trainer.ini";

void Trim(char* s) {
    // strip trailing CR/LF/whitespace in place
    size_t len = strlen(s);
    while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r' || s[len - 1] == ' ')) {
        s[--len] = '\0';
    }
}
}  // namespace

void LoadConfig() {
    FILE* f = fopen(kConfigFileName, "r");
    if (!f) {
        Log("[Config] No existing %s found, using defaults.", kConfigFileName);
        return;
    }

    auto& settings = GetTrainerSettings();
    auto& vars = GetTrainerVars();
    auto& hotkeys = GetTrainerHotkeys();

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        Trim(line);
        char* eq = strchr(line, '=');
        if (!eq) continue;
        *eq = '\0';
        const char* key = line;
        const char* value = eq + 1;

        auto asBool = [&]() { return atoi(value) != 0; };
        auto asInt = [&]() { return atoi(value); };
        auto asFloat = [&]() { return static_cast<float>(atof(value)); };
        auto asU32 = [&]() { return static_cast<uint32_t>(strtoul(value, nullptr, 10)); };
        auto asU64Hex = [&]() { return static_cast<uint64_t>(strtoull(value, nullptr, 16)); };

        if (!strcmp(key, "infiniteHealth")) settings.infiniteHealth = asBool();
        else if (!strcmp(key, "infiniteResolve")) settings.infiniteResolve = asBool();
        else if (!strcmp(key, "stealth")) settings.stealth = asBool();
        else if (!strcmp(key, "noIndicator")) settings.noIndicator = asBool();
        else if (!strcmp(key, "forcePerfectDodge")) settings.forcePerfectDodge = asBool();
        else if (!strcmp(key, "autoBlockMelee")) settings.autoBlockMelee = asBool();
        else if (!strcmp(key, "alwaysParryOnBlock")) settings.alwaysParryOnBlock = asBool();
        else if (!strcmp(key, "perfectParry")) settings.perfectParry = asBool();
        else if (!strcmp(key, "movementSpeed")) settings.movementSpeed = asBool();
        else if (!strcmp(key, "pickupMultiplier")) settings.pickupMultiplier = asBool();
        else if (!strcmp(key, "freeCam")) settings.freeCam = asBool();
        else if (!strcmp(key, "gearUnlockOnLoad")) settings.gearUnlockOnLoad = asBool();
        else if (!strcmp(key, "autoSaveConfig")) settings.autoSaveConfig = asBool();
        else if (!strcmp(key, "showToasts")) settings.showToasts = asBool();
        else if (!strcmp(key, "freeCamSpeed")) vars.freeCamSpeed = asFloat();
        else if (!strcmp(key, "movementSpeedBonus")) vars.movementSpeedBonus = asFloat();
        else if (!strcmp(key, "resourceAmount")) vars.resourceAmount = asU32();
        else if (!strcmp(key, "pickupMultiplierValue")) vars.pickupMultiplier = asU32();
        else if (!strcmp(key, "gearHash")) vars.gearHash = asU64Hex();
        else if (!strcmp(key, "hkMenuToggle")) hotkeys.menuToggle = asInt();
        else if (!strcmp(key, "hkInfiniteHealth")) hotkeys.infiniteHealth = asInt();
        else if (!strcmp(key, "hkStealth")) hotkeys.stealth = asInt();
        else if (!strcmp(key, "hkFreeCam")) hotkeys.freeCam = asInt();
        else if (!strcmp(key, "hkPanic")) hotkeys.panic = asInt();
    }

    fclose(f);
    Log("[Config] Loaded settings from %s", kConfigFileName);
}

void SaveConfig() {
    FILE* f = fopen(kConfigFileName, "w");
    if (!f) {
        Log("[Config] ERROR: failed to write %s", kConfigFileName);
        return;
    }

    const auto& settings = GetTrainerSettings();
    const auto& vars = GetTrainerVars();
    const auto& hotkeys = GetTrainerHotkeys();

    fprintf(f, "; Ghost of Tsushima Trainer settings - auto-generated, safe to hand-edit\n");
    fprintf(f, "infiniteHealth=%d\n", settings.infiniteHealth);
    fprintf(f, "infiniteResolve=%d\n", settings.infiniteResolve);
    fprintf(f, "stealth=%d\n", settings.stealth);
    fprintf(f, "noIndicator=%d\n", settings.noIndicator);
    fprintf(f, "forcePerfectDodge=%d\n", settings.forcePerfectDodge);
    fprintf(f, "autoBlockMelee=%d\n", settings.autoBlockMelee);
    fprintf(f, "alwaysParryOnBlock=%d\n", settings.alwaysParryOnBlock);
    fprintf(f, "perfectParry=%d\n", settings.perfectParry);
    fprintf(f, "movementSpeed=%d\n", settings.movementSpeed);
    fprintf(f, "pickupMultiplier=%d\n", settings.pickupMultiplier);
    fprintf(f, "freeCam=%d\n", settings.freeCam);
    fprintf(f, "gearUnlockOnLoad=%d\n", settings.gearUnlockOnLoad);
    fprintf(f, "autoSaveConfig=%d\n", settings.autoSaveConfig);
    fprintf(f, "showToasts=%d\n", settings.showToasts);
    fprintf(f, "freeCamSpeed=%.4f\n", vars.freeCamSpeed);
    fprintf(f, "movementSpeedBonus=%.4f\n", vars.movementSpeedBonus);
    fprintf(f, "resourceAmount=%u\n", vars.resourceAmount);
    fprintf(f, "pickupMultiplierValue=%u\n", vars.pickupMultiplier);
    fprintf(f, "gearHash=%016llX\n", static_cast<unsigned long long>(vars.gearHash));
    fprintf(f, "hkMenuToggle=%d\n", hotkeys.menuToggle);
    fprintf(f, "hkInfiniteHealth=%d\n", hotkeys.infiniteHealth);
    fprintf(f, "hkStealth=%d\n", hotkeys.stealth);
    fprintf(f, "hkFreeCam=%d\n", hotkeys.freeCam);
    fprintf(f, "hkPanic=%d\n", hotkeys.panic);

    fclose(f);
    Log("[Config] Saved settings to %s", kConfigFileName);
}

}  // namespace got
