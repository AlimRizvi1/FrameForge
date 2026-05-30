#include "dx11_hook.h"
#include <MinHook.h>
#include <iostream>
#include <vector>
#include "../engine/frame_capture.h"
#include "../engine/pacing_controller.h"
#include "../overlay/overlay.h"

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace FrameForge::Hooks {

    typedef HRESULT(STDMETHODCALLTYPE* Present)(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
    Present OriginalPresent = nullptr;

    FrameForge::Engine::FrameCapture* g_FrameCapture = nullptr;
    FrameForge::Engine::PacingController* g_PacingController = nullptr;
    FrameForge::Overlay::Manager* g_OverlayManager = nullptr;
    HWND g_hWindow = nullptr;
    WNDPROC g_OriginalWndProc = nullptr;

    LRESULT CALLBACK HookedWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
            return true;
        return CallWindowProc(g_OriginalWndProc, hWnd, msg, wParam, lParam);
    }

    HRESULT STDMETHODCALLTYPE HookedPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags) {
        if (!g_FrameCapture) {
            g_FrameCapture = new FrameForge::Engine::FrameCapture();
        }

        if (!g_PacingController) {
            g_PacingController = new FrameForge::Engine::PacingController();
            g_PacingController->SetTargetFPS(60); // Default
        }

        if (!g_OverlayManager) {
            g_OverlayManager = new FrameForge::Overlay::Manager();
            g_OverlayManager->Initialize(pSwapChain);

            DXGI_SWAP_CHAIN_DESC sd;
            pSwapChain->GetDesc(&sd);
            g_hWindow = sd.OutputWindow;
            g_OriginalWndProc = reinterpret_cast<WNDPROC>(SetWindowLongPtr(g_hWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(HookedWndProc)));
        }

        g_PacingController->Update();
        g_FrameCapture->Capture(pSwapChain);

        // TODO: Frame Interpolation logic here

        g_OverlayManager->Render();

        return OriginalPresent(pSwapChain, SyncInterval, Flags);
    }

    bool Initialize() {
        if (MH_Initialize() != MH_OK) return false;

        // Create a dummy device and swapchain to get the VTable
        ID3D11Device* pDevice = nullptr;
        ID3D11DeviceContext* pContext = nullptr;
        IDXGISwapChain* pSwapChain = nullptr;

        D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
        DXGI_SWAP_CHAIN_DESC scd = {};
        scd.BufferCount = 1;
        scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        scd.OutputWindow = GetForegroundWindow(); // Temporary
        scd.SampleDesc.Count = 1;
        scd.Windowed = TRUE;
        scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

        HRESULT hr = D3D11CreateDeviceAndSwapChain(
            nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, &featureLevel, 1,
            D3D11_SDK_VERSION, &scd, &pSwapChain, &pDevice, nullptr, &pContext
        );

        if (FAILED(hr)) {
            std::cerr << "[FrameForge] Failed to create dummy DX11 device." << std::endl;
            return false;
        }

        void** vtable = *reinterpret_cast<void***>(pSwapChain);
        void* present_addr = vtable[8]; // IDXGISwapChain::Present is index 8

        if (MH_CreateHook(present_addr, &HookedPresent, reinterpret_cast<LPVOID*>(&OriginalPresent)) != MH_OK) {
            std::cerr << "[FrameForge] Failed to create Present hook." << std::endl;
            return false;
        }

        if (MH_EnableHook(present_addr) != MH_OK) {
            std::cerr << "[FrameForge] Failed to enable Present hook." << std::endl;
            return false;
        }

        pSwapChain->Release();
        pDevice->Release();
        pContext->Release();

        return true;
    }

    void Shutdown() {
        MH_DisableHook(MH_ALL_HOOKS);
        MH_Uninitialize();
    }
}
