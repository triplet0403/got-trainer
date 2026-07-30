#include "core/hook_manager.hpp"

#include <cstring>

namespace got {

HookManager& HookManager::Instance() {
    static HookManager inst;
    return inst;
}

bool HookManager::Initialize() {
    if (initialized_) {
        return true;
    }
    if (MH_Initialize() != MH_OK) {
        return false;
    }
    initialized_ = true;
    return true;
}

void HookManager::Shutdown() {
    std::lock_guard lock(patchMutex_);
    for (auto& [tag, backup] : patches_) {
        if (backup.address && !backup.original.empty()) {
            DWORD oldProtect = 0;
            VirtualProtect(backup.address, backup.original.size(), PAGE_EXECUTE_READWRITE, &oldProtect);
            std::memcpy(backup.address, backup.original.data(), backup.original.size());
            VirtualProtect(backup.address, backup.original.size(), oldProtect, &oldProtect);
        }
        (void)tag;
    }
    patches_.clear();

    if (initialized_) {
        MH_DisableHook(MH_ALL_HOOKS);
        MH_Uninitialize();
        initialized_ = false;
    }
}

bool HookManager::CreateHook(void* target, void* detour, void** original) {
    if (!initialized_ && !Initialize()) {
        return false;
    }
    if (MH_CreateHook(target, detour, original) != MH_OK) {
        return false;
    }
    return true;
}

bool HookManager::EnableHook(void* target) {
    return MH_EnableHook(target) == MH_OK;
}

bool HookManager::DisableHook(void* target) {
    return MH_DisableHook(target) == MH_OK;
}

bool HookManager::PatchBytes(void* address, const uint8_t* bytes, size_t len, void* tag) {
    if (!address || !bytes || len == 0) {
        return false;
    }
    std::lock_guard lock(patchMutex_);
    PatchBackup backup;
    backup.address = address;
    backup.original.resize(len);
    std::memcpy(backup.original.data(), address, len);

    DWORD oldProtect = 0;
    if (!VirtualProtect(address, len, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        return false;
    }
    std::memcpy(address, bytes, len);
    VirtualProtect(address, len, oldProtect, &oldProtect);
    patches_[tag] = std::move(backup);
    return true;
}

bool HookManager::RestoreBytes(void* tag) {
    std::lock_guard lock(patchMutex_);
    auto it = patches_.find(tag);
    if (it == patches_.end()) {
        return false;
    }
    auto& backup = it->second;
    DWORD oldProtect = 0;
    VirtualProtect(backup.address, backup.original.size(), PAGE_EXECUTE_READWRITE, &oldProtect);
    std::memcpy(backup.address, backup.original.data(), backup.original.size());
    VirtualProtect(backup.address, backup.original.size(), oldProtect, &oldProtect);
    patches_.erase(it);
    return true;
}

}  // namespace got
