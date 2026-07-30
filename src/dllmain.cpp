#include <Windows.h>
#include <atomic>
#include <thread>

#include "core/config.hpp"
#include "core/game_pointers.hpp"
#include "core/hook_manager.hpp"
#include "core/hotkeys.hpp"
#include "core/log.hpp"
#include "core/trainer_state.hpp"
#include "features/feature_registry.hpp"
#include "proxy/version_proxy.hpp"
#include "render/dx11_present_hook.hpp"

namespace {

std::thread g_mainThread;
std::atomic<bool> g_running{false};

void MainThread() {
    got::Log("[Trainer] MainThread started. Waiting for GhostOfTsushima.exe module...");

    while (!GetModuleHandleW(L"GhostOfTsushima.exe")) {
        Sleep(100);
    }
    got::Log("[Trainer] GhostOfTsushima.exe module handle found. Sleeping 2s for engine init...");
    Sleep(2000);

    got::Log("[Trainer] Initializing MinHook...");
    if (got::HookManager::Instance().Initialize()) {
        got::Log("[Trainer] MinHook initialized successfully.");
    } else {
        got::Log("[Trainer] ERROR: MinHook initialization failed!");
    }

    got::Log("[Trainer] Running AOB pattern scans...");
    got::GetGamePointers().Rescan();
    got::Log("[Trainer] Pattern scans complete.");

    got::Log("[Trainer] Initializing Present Hook...");
    got::InitPresentHook();
    got::Log("[Trainer] Present Hook initialized.");

    got::Log("[Trainer] Initializing Feature Registry...");
    got::FeatureRegistry::Instance().Initialize();

    got::LoadConfig();

    got::Log("[Trainer] Worker thread running. Press the menu hotkey to open the overlay.");

    while (g_running.load()) {
        got::ProcessHotkeys();
        Sleep(15);
    }

    if (got::GetTrainerSettings().autoSaveConfig) {
        got::SaveConfig();
    }
    got::Log("[Trainer] MainThread exiting.");
}

}  // namespace

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID reserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        got::InitLog();
        got::Log("[Trainer] DllMain: DLL_PROCESS_ATTACH received.");

        if (got::InitProxy()) {
            got::Log("[Trainer] Proxy initialized successfully (System32 version.dll loaded).");
        } else {
            got::Log("[Trainer] ERROR: Proxy initialization failed!");
        }

        g_running.store(true);
        g_mainThread = std::thread(MainThread);
    } else if (reason == DLL_PROCESS_DETACH) {
        got::Log("[Trainer] DllMain: DLL_PROCESS_DETACH received.");
        g_running.store(false);
        if (g_mainThread.joinable()) {
            g_mainThread.join();
        }

        got::FeatureRegistry::Instance().Shutdown();
        got::ShutdownPresentHook();
        got::HookManager::Instance().Shutdown();
        got::ShutdownProxy();
        got::Log("[Trainer] Shutdown complete.");
        got::CloseLog();
    }
    return TRUE;
}
