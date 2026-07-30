#include "render/dx11_present_hook.hpp"

#include <atomic>
#include <d3d11.h>
#include <d3d12.h>
#include <dxgi.h>
#include <dxgi1_4.h>
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_dx12.h>
#include <imgui_impl_win32.h>
#include <vector>

#include "core/hook_manager.hpp"
#include "core/log.hpp"
#include "core/notify.hpp"
#include "core/trainer_state.hpp"
#include "features/feature_registry.hpp"
#include "ui/trainer_ui.hpp"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace got {
namespace {

//--------------------------------------------------------------------
// Hook types
//--------------------------------------------------------------------
using PresentFn             = HRESULT(WINAPI*)(IDXGISwapChain*, UINT, UINT);
using ExecuteCommandListsFn = void(WINAPI*)(ID3D12CommandQueue*, UINT, ID3D12CommandList* const*);

PresentFn             g_originalPresent             = nullptr;
ExecuteCommandListsFn g_originalExecuteCommandLists = nullptr;

//--------------------------------------------------------------------
// State
//--------------------------------------------------------------------
HWND    g_hwnd            = nullptr;
WNDPROC g_originalWndProc = nullptr;
bool    g_initialized     = false;
bool    g_isDX12          = false;
bool    g_initFailed      = false;

std::atomic<ID3D12CommandQueue*> g_gameCommandQueue{nullptr};

//--------------------------------------------------------------------
// DX11
//--------------------------------------------------------------------
ID3D11Device*           g_d3d11Device           = nullptr;
ID3D11DeviceContext*    g_d3d11Context          = nullptr;
ID3D11RenderTargetView* g_d3d11RenderTargetView = nullptr;

//--------------------------------------------------------------------
// DX12
//--------------------------------------------------------------------
struct FrameContext {
    ID3D12CommandAllocator* commandAllocator = nullptr;
    UINT64                  fenceValue       = 0;
};

ID3D12Device*                  g_d3d12Device  = nullptr;
ID3D12DescriptorHeap*          g_srvHeap      = nullptr;
ID3D12DescriptorHeap*          g_rtvHeap      = nullptr;
std::vector<ID3D12Resource*>   g_rtvBuffers;
std::vector<FrameContext>      g_frameContexts;
ID3D12GraphicsCommandList*     g_cmdList      = nullptr;
ID3D12Fence*                   g_fence        = nullptr;
HANDLE                         g_fenceEvent   = nullptr;
UINT64                         g_fenceValue   = 0;
UINT                           g_bufferCount  = 0;

//--------------------------------------------------------------------
// WndProc – configurable hotkey toggles the menu (default F8)
//--------------------------------------------------------------------
LRESULT CALLBACK HookedWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_KEYDOWN && static_cast<int>(wParam) == GetTrainerHotkeys().menuToggle) {
        auto& s = GetTrainerSettings();
        s.menuOpen = !s.menuOpen;
        Log("[UI] Menu toggled -> %s", s.menuOpen ? "OPEN" : "CLOSED");
        return 0;
    }

    if (GetTrainerSettings().menuOpen) {
        ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam);
        switch (msg) {
            case WM_KEYDOWN:    case WM_KEYUP:
            case WM_SYSKEYDOWN: case WM_SYSKEYUP:
            case WM_CHAR:
            case WM_LBUTTONDOWN: case WM_LBUTTONUP:
            case WM_RBUTTONDOWN: case WM_RBUTTONUP:
            case WM_MBUTTONDOWN: case WM_MBUTTONUP:
            case WM_MOUSEWHEEL: case WM_MOUSEMOVE:
                return 0;
        }
    }
    return CallWindowProcW(g_originalWndProc, hwnd, msg, wParam, lParam);
}

//--------------------------------------------------------------------
// ExecuteCommandLists hook – capture game's real D3D12 queue
//--------------------------------------------------------------------
void WINAPI HookedExecuteCommandLists(
    ID3D12CommandQueue*       pCommandQueue,
    UINT                      NumCommandLists,
    ID3D12CommandList* const* ppCommandLists)
{
    if (pCommandQueue) {
        D3D12_COMMAND_QUEUE_DESC d = pCommandQueue->GetDesc();
        if (d.Type == D3D12_COMMAND_LIST_TYPE_DIRECT)
            g_gameCommandQueue.store(pCommandQueue);
    }
    g_originalExecuteCommandLists(pCommandQueue, NumCommandLists, ppCommandLists);
}

//--------------------------------------------------------------------
// DX12 init
//--------------------------------------------------------------------
bool InitImGuiDX12(IDXGISwapChain* pSwapChain) {
    ID3D12CommandQueue* gameQueue = g_gameCommandQueue.load();
    if (!gameQueue) return false;  // wait until queue captured

    Log("[Render] InitImGuiDX12 – gameQueue: 0x%p", gameQueue);

    HRESULT hr = gameQueue->GetDevice(IID_PPV_ARGS(&g_d3d12Device));
    if (FAILED(hr) || !g_d3d12Device) { Log("[Render] GetDevice failed: 0x%lX", hr); return false; }

    DXGI_SWAP_CHAIN_DESC sc = {};
    pSwapChain->GetDesc(&sc);
    g_hwnd        = sc.OutputWindow;
    g_bufferCount = sc.BufferCount;

    DXGI_FORMAT fmt = sc.BufferDesc.Format;
    if (fmt == DXGI_FORMAT_UNKNOWN) {
        IDXGISwapChain1* sc1 = nullptr;
        if (SUCCEEDED(pSwapChain->QueryInterface(IID_PPV_ARGS(&sc1)))) {
            DXGI_SWAP_CHAIN_DESC1 d1 = {};
            sc1->GetDesc1(&d1);
            fmt = d1.Format;
            sc1->Release();
        }
        if (fmt == DXGI_FORMAT_UNKNOWN) fmt = DXGI_FORMAT_R8G8B8A8_UNORM;
    }
    Log("[Render] HWND=%p  BufferCount=%u  Format=%d", g_hwnd, g_bufferCount, fmt);

    // SRV heap
    D3D12_DESCRIPTOR_HEAP_DESC srvDesc = {};
    srvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV; srvDesc.NumDescriptors = 1;
    srvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    if (FAILED(g_d3d12Device->CreateDescriptorHeap(&srvDesc, IID_PPV_ARGS(&g_srvHeap)))) {
        Log("[Render] SRV heap failed"); return false;
    }

    // RTV heap
    D3D12_DESCRIPTOR_HEAP_DESC rtvDesc = {};
    rtvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV; rtvDesc.NumDescriptors = g_bufferCount;
    if (FAILED(g_d3d12Device->CreateDescriptorHeap(&rtvDesc, IID_PPV_ARGS(&g_rtvHeap)))) {
        Log("[Render] RTV heap failed"); return false;
    }

    SIZE_T rtvStride = g_d3d12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = g_rtvHeap->GetCPUDescriptorHandleForHeapStart();

    g_rtvBuffers.resize(g_bufferCount, nullptr);
    g_frameContexts.resize(g_bufferCount);

    for (UINT i = 0; i < g_bufferCount; ++i) {
        if (FAILED(pSwapChain->GetBuffer(i, IID_PPV_ARGS(&g_rtvBuffers[i])))) {
            Log("[Render] GetBuffer(%u) failed", i); return false;
        }
        D3D12_RENDER_TARGET_VIEW_DESC rtvView = {};
        rtvView.Format        = fmt;
        rtvView.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
        g_d3d12Device->CreateRenderTargetView(g_rtvBuffers[i], &rtvView, rtvHandle);
        rtvHandle.ptr += rtvStride;

        if (FAILED(g_d3d12Device->CreateCommandAllocator(
                D3D12_COMMAND_LIST_TYPE_DIRECT,
                IID_PPV_ARGS(&g_frameContexts[i].commandAllocator)))) {
            Log("[Render] CommandAllocator(%u) failed", i); return false;
        }
        g_frameContexts[i].fenceValue = 0;
    }

    if (FAILED(g_d3d12Device->CreateCommandList(
            0, D3D12_COMMAND_LIST_TYPE_DIRECT,
            g_frameContexts[0].commandAllocator, nullptr, IID_PPV_ARGS(&g_cmdList)))) {
        Log("[Render] CreateCommandList failed"); return false;
    }
    g_cmdList->Close();

    // Fence for GPU sync
    g_fenceValue = 0;
    if (FAILED(g_d3d12Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&g_fence)))) {
        Log("[Render] CreateFence failed"); return false;
    }
    g_fenceEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    if (!g_fenceEvent) { Log("[Render] CreateEvent failed"); return false; }

    // ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr;
    ImGui::StyleColorsDark();
    ApplyTrainerTheme();

    ImGui_ImplWin32_Init(g_hwnd);
    ImGui_ImplDX12_Init(g_d3d12Device, g_bufferCount, fmt, g_srvHeap,
        g_srvHeap->GetCPUDescriptorHandleForHeapStart(),
        g_srvHeap->GetGPUDescriptorHandleForHeapStart());

    g_originalWndProc = reinterpret_cast<WNDPROC>(
        SetWindowLongPtrW(g_hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(HookedWndProc)));

    g_isDX12 = true; g_initialized = true;
    Log("[Render] DX12 ImGui OK.");
    return true;
}

//--------------------------------------------------------------------
// DX11 fallback
//--------------------------------------------------------------------
bool InitImGuiDX11(IDXGISwapChain* pSwapChain) {
    Log("[Render] InitImGuiDX11...");
    if (FAILED(pSwapChain->GetDevice(IID_PPV_ARGS(&g_d3d11Device)))) { Log("[Render] GetDevice(DX11) failed"); return false; }
    g_d3d11Device->GetImmediateContext(&g_d3d11Context);
    DXGI_SWAP_CHAIN_DESC sc = {}; pSwapChain->GetDesc(&sc);
    g_hwnd = sc.OutputWindow;
    ID3D11Texture2D* bb = nullptr;
    if (SUCCEEDED(pSwapChain->GetBuffer(0, IID_PPV_ARGS(&bb))) && bb) {
        g_d3d11Device->CreateRenderTargetView(bb, nullptr, &g_d3d11RenderTargetView);
        bb->Release();
    }
    IMGUI_CHECKVERSION(); ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr;
    ImGui::StyleColorsDark();
    ApplyTrainerTheme();
    ImGui_ImplWin32_Init(g_hwnd);
    ImGui_ImplDX11_Init(g_d3d11Device, g_d3d11Context);
    g_originalWndProc = reinterpret_cast<WNDPROC>(
        SetWindowLongPtrW(g_hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(HookedWndProc)));
    g_isDX12 = false; g_initialized = true;
    Log("[Render] DX11 ImGui OK."); return true;
}

//--------------------------------------------------------------------
// DX12 per-frame render
// Key insight: game transitions backbuffer to D3D12_RESOURCE_STATE_PRESENT
// before calling Present. We must barrier to RENDER_TARGET, draw, then
// barrier back to PRESENT. All submitted on the game's real queue.
//--------------------------------------------------------------------
void RenderDX12(IDXGISwapChain* pSwapChain) {
    ID3D12CommandQueue* gameQueue = g_gameCommandQueue.load();
    if (!gameQueue || !g_cmdList || !g_fence) return;

    IDXGISwapChain3* sc3 = nullptr;
    if (FAILED(pSwapChain->QueryInterface(IID_PPV_ARGS(&sc3))) || !sc3) return;
    UINT idx = sc3->GetCurrentBackBufferIndex();
    sc3->Release();
    if (idx >= g_bufferCount) return;

    // Wait for this frame's GPU work to complete before resetting allocator
    FrameContext& frame = g_frameContexts[idx];
    if (frame.fenceValue != 0 && g_fence->GetCompletedValue() < frame.fenceValue) {
        g_fence->SetEventOnCompletion(frame.fenceValue, g_fenceEvent);
        WaitForSingleObject(g_fenceEvent, 200);
    }

    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    if (GetTrainerSettings().menuOpen) RenderTrainerUI();
    if (GetTrainerSettings().showToasts) DrawToasts();
    ImGui::EndFrame();
    ImGui::Render();

    frame.commandAllocator->Reset();
    g_cmdList->Reset(frame.commandAllocator, nullptr);

    // PRESENT → RENDER_TARGET
    D3D12_RESOURCE_BARRIER barrierToRT = {};
    barrierToRT.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrierToRT.Transition.pResource   = g_rtvBuffers[idx];
    barrierToRT.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrierToRT.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrierToRT.Transition.StateAfter  = D3D12_RESOURCE_STATE_RENDER_TARGET;
    g_cmdList->ResourceBarrier(1, &barrierToRT);

    SIZE_T rtvStride = g_d3d12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    D3D12_CPU_DESCRIPTOR_HANDLE rtv = g_rtvHeap->GetCPUDescriptorHandleForHeapStart();
    rtv.ptr += idx * rtvStride;

    g_cmdList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
    g_cmdList->SetDescriptorHeaps(1, &g_srvHeap);
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), g_cmdList);

    // RENDER_TARGET → PRESENT
    D3D12_RESOURCE_BARRIER barrierToPresent  = barrierToRT;
    barrierToPresent.Transition.StateBefore  = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrierToPresent.Transition.StateAfter   = D3D12_RESOURCE_STATE_PRESENT;
    g_cmdList->ResourceBarrier(1, &barrierToPresent);

    g_cmdList->Close();

    // Submit on game's real queue (same GPU timeline as Present)
    ID3D12CommandList* lists[] = {g_cmdList};
    gameQueue->ExecuteCommandLists(1, lists);

    // Signal fence so we can wait next time this frame index comes around
    ++g_fenceValue;
    gameQueue->Signal(g_fence, g_fenceValue);
    frame.fenceValue = g_fenceValue;
}

//--------------------------------------------------------------------
// DX11 render
//--------------------------------------------------------------------
void RenderDX11() {
    ImGui_ImplDX11_NewFrame(); ImGui_ImplWin32_NewFrame(); ImGui::NewFrame();
    if (GetTrainerSettings().menuOpen) RenderTrainerUI();
    if (GetTrainerSettings().showToasts) DrawToasts();
    ImGui::EndFrame(); ImGui::Render();
    g_d3d11Context->OMSetRenderTargets(1, &g_d3d11RenderTargetView, nullptr);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

//--------------------------------------------------------------------
// Hooked Present
//--------------------------------------------------------------------
HRESULT WINAPI HookedPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags) {
    if (!g_initialized && !g_initFailed) {
        bool ok = InitImGuiDX12(pSwapChain);
        if (!ok && g_gameCommandQueue.load()) {
            if (!InitImGuiDX11(pSwapChain)) {
                Log("[Render] All init failed."); g_initFailed = true;
            }
        }
    }
    if (g_initialized) {
        FeatureRegistry::Instance().Update();
        if (g_isDX12) RenderDX12(pSwapChain);
        else          RenderDX11();
    }
    return g_originalPresent(pSwapChain, SyncInterval, Flags);
}

} // anonymous namespace

//--------------------------------------------------------------------
// Public API
//--------------------------------------------------------------------
void InitPresentHook() {
    Log("[Hook] Setting up Present + ExecuteCommandLists hooks...");

    // Present hook via dummy DX11 swapchain
    HWND hw = CreateWindowExA(0, "STATIC", "GOTDummy", WS_POPUP, 0, 0, 2, 2, nullptr, nullptr, nullptr, nullptr);
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 1; sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.Width = 2; sd.BufferDesc.Height = 2;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hw ? hw : GetDesktopWindow();
    sd.SampleDesc.Count = 1; sd.Windowed = TRUE; sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    ID3D11Device* dev = nullptr; IDXGISwapChain* sc = nullptr;
    ID3D11DeviceContext* ctx = nullptr; D3D_FEATURE_LEVEL fl = {};
    HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &sd, &sc, &dev, &fl, &ctx);
    if (FAILED(hr)) hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &sd, &sc, &dev, &fl, &ctx);

    if (SUCCEEDED(hr) && sc) {
        void* presentFn = (*reinterpret_cast<void***>(sc))[8];
        sc->Release(); dev->Release(); ctx->Release();
        if (hw) DestroyWindow(hw);
        auto& hm = HookManager::Instance();
        if (hm.CreateHook(presentFn, reinterpret_cast<void*>(HookedPresent), reinterpret_cast<void**>(&g_originalPresent))) {
            hm.EnableHook(presentFn);
            Log("[Hook] Present hooked at 0x%p", presentFn);
        } else Log("[Hook] ERROR: Present hook failed.");
    } else { if (hw) DestroyWindow(hw); Log("[Hook] ERROR: Dummy swapchain failed."); }

    // ExecuteCommandLists hook via dummy DX12 queue
    ID3D12Device* d12 = nullptr;
    hr = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&d12));
    if (SUCCEEDED(hr) && d12) {
        D3D12_COMMAND_QUEUE_DESC qd = { D3D12_COMMAND_LIST_TYPE_DIRECT, 0, D3D12_COMMAND_QUEUE_FLAG_NONE, 0 };
        ID3D12CommandQueue* dq = nullptr;
        if (SUCCEEDED(d12->CreateCommandQueue(&qd, IID_PPV_ARGS(&dq)))) {
            void* execFn = (*reinterpret_cast<void***>(dq))[10];
            dq->Release(); d12->Release();
            auto& hm = HookManager::Instance();
            if (hm.CreateHook(execFn, reinterpret_cast<void*>(HookedExecuteCommandLists), reinterpret_cast<void**>(&g_originalExecuteCommandLists))) {
                hm.EnableHook(execFn);
                Log("[Hook] ExecuteCommandLists hooked at 0x%p", execFn);
            } else Log("[Hook] ERROR: ECL hook failed.");
        } else { d12->Release(); }
    }
}

void ShutdownPresentHook() {
    if (g_originalWndProc && g_hwnd)
        SetWindowLongPtrW(g_hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(g_originalWndProc));

    // Wait for GPU to finish
    if (g_fence && g_fenceEvent && g_gameCommandQueue.load()) {
        ID3D12CommandQueue* q = g_gameCommandQueue.load();
        ++g_fenceValue;
        q->Signal(g_fence, g_fenceValue);
        g_fence->SetEventOnCompletion(g_fenceValue, g_fenceEvent);
        WaitForSingleObject(g_fenceEvent, 1000);
    }

    if (g_initialized) {
        if (g_isDX12) ImGui_ImplDX12_Shutdown();
        else          ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
    }
    for (auto* rt : g_rtvBuffers)              if (rt) rt->Release();
    for (auto& fc : g_frameContexts)           if (fc.commandAllocator) fc.commandAllocator->Release();
    if (g_cmdList)               g_cmdList->Release();
    if (g_fence)                 g_fence->Release();
    if (g_fenceEvent)            CloseHandle(g_fenceEvent);
    if (g_srvHeap)               g_srvHeap->Release();
    if (g_rtvHeap)               g_rtvHeap->Release();
    if (g_d3d12Device)           g_d3d12Device->Release();
    if (g_d3d11RenderTargetView) g_d3d11RenderTargetView->Release();
    if (g_d3d11Context)          g_d3d11Context->Release();
    if (g_d3d11Device)           g_d3d11Device->Release();
    g_initialized = false;
}

bool IsMenuOpen() { return GetTrainerSettings().menuOpen; }

} // namespace got
