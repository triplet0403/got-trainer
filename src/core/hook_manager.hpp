#pragma once

#include <MinHook.h>
#include <Windows.h>
#include <cstdint>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace got {

class HookManager {
public:
    static HookManager& Instance();

    bool Initialize();
    void Shutdown();

    bool CreateHook(void* target, void* detour, void** original);
    bool EnableHook(void* target);
    bool DisableHook(void* target);

    bool PatchBytes(void* address, const uint8_t* bytes, size_t len, void* tag);
    bool RestoreBytes(void* tag);

private:
    HookManager() = default;
    bool initialized_ = false;

    struct PatchBackup {
        void* address = nullptr;
        std::vector<uint8_t> original;
    };
    std::mutex patchMutex_;
    std::unordered_map<void*, PatchBackup> patches_;
};

}  // namespace got
