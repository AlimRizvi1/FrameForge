#include "dx11_hook.h"
#include <MinHook.h>
#include <iostream>
#include <vector>
#include <string>
#include "../engine/frame_capture.h"
#include "../engine/pacing_controller.h"
#include "../engine/interpolation_engine.h"
#include "../engine/motion_estimator.h"
#include "../overlay/overlay.h"

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
void LogToFile(const char* msg);

namespace FrameForge::Hooks {

    typedef HRESULT(STDMETHODCALLTYPE* Present)(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
    Present OriginalPresent = nullptr;
    typedef HRESULT(STDMETHODCALLTYPE* ResizeBuffers)(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);
    ResizeBuffers OriginalResizeBuffers = nullptr;

    FrameForge::Engine::FrameCapture* g_FrameCapture = nullptr;
    FrameForge::Engine::PacingController* g_PacingController = nullptr;
    FrameForge::Engine::InterpolationEngine* g_InterpolationEngine = nullptr;
    FrameForge::Engine::MotionEstimator* g_MotionEstimator = nullptr;
    FrameForge::Overlay::Manager* g_OverlayManager = nullptr;
    
    ID3D11Texture2D* g_InterpolatedFrame = nullptr;
    ID3D11Texture2D* g_MotionVectors = nullptr;
    HWND g_hWindow = nullptr;
    WNDPROC g_OriginalWndProc = nullptr;

    void CleanupResources() {
        if (g_OverlayManager) { g_OverlayManager->Shutdown(); delete g_OverlayManager; g_OverlayManager = nullptr; }
        if (g_InterpolatedFrame) { g_InterpolatedFrame->Release(); g_InterpolatedFrame = nullptr; }
        if (g_MotionVectors) { g_MotionVectors->Release(); g_MotionVectors = nullptr; }
        if (g_FrameCapture) { delete g_FrameCapture; g_FrameCapture = nullptr; }
    }

    LRESULT CALLBACK HookedWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) return true;
        return CallWindowProc(g_OriginalWndProc, hWnd, msg, wParam, lParam);
    }

    HRESULT STDMETHODCALLTYPE HookedResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags) {
        CleanupResources();
        return OriginalResizeBuffers(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);
    }

    HRESULT STDMETHODCALLTYPE HookedPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags) {
        if (!g_FrameCapture) g_FrameCapture = new FrameForge::Engine::FrameCapture();
        if (!g_PacingController) g_PacingController = new FrameForge::Engine::PacingController();

        ID3D11Device* pDevice = nullptr;
        pSwapChain->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(&pDevice));
        ID3D11DeviceContext* pContext = nullptr;
        pDevice->GetImmediateContext(&pContext);

        if (!g_InterpolationEngine) {
            g_InterpolationEngine = new FrameForge::Engine::InterpolationEngine();
            g_InterpolationEngine->Initialize(pDevice);
        }
        if (!g_MotionEstimator) {
            g_MotionEstimator = new FrameForge::Engine::MotionEstimator();
            g_MotionEstimator->Initialize(pDevice);
        }
        if (!g_OverlayManager) {
            g_OverlayManager = new FrameForge::Overlay::Manager();
            if (g_OverlayManager->Initialize(pSwapChain)) {
                DXGI_SWAP_CHAIN_DESC sd; pSwapChain->GetDesc(&sd);
                g_hWindow = sd.OutputWindow;
                g_OriginalWndProc = reinterpret_cast<WNDPROC>(SetWindowLongPtr(g_hWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(HookedWndProc)));
            }
        }

        // 1. SMART INSERTION: Check if we need to fill a timing gap
        float dynamicWeight = 0.5f;
        if (g_FrameCapture->GetPreviousFrame() && g_PacingController->ShouldInsertFrame(dynamicWeight)) {
            if (!g_InterpolatedFrame) g_InterpolatedFrame = g_FrameCapture->CreateOutputTexture();
            if (!g_MotionVectors) g_MotionVectors = g_FrameCapture->CreateMotionVectorTexture();

            // A. Truly Motion-Aware Warping
            g_MotionEstimator->Estimate(pContext, g_FrameCapture->GetPreviousFrame(), g_FrameCapture->GetCurrentFrame(), g_MotionVectors);
            g_InterpolationEngine->Process(pContext, g_FrameCapture->GetPreviousFrame(), g_FrameCapture->GetCurrentFrame(), g_MotionVectors, g_InterpolatedFrame, dynamicWeight);

            // B. Present ONLY the interpolated frame
            ID3D11Texture2D* pBackBuffer = nullptr;
            if (SUCCEEDED(pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pBackBuffer)))) {
                pContext->CopyResource(pBackBuffer, g_InterpolatedFrame);
                pBackBuffer->Release();
            }
            OriginalPresent(pSwapChain, SyncInterval, Flags);
            g_PacingController->UpdateDisplay();
        }

        // 2. REGULAR PATH: Game finished a frame
        g_PacingController->UpdateRender();
        g_FrameCapture->Capture(pSwapChain);

        if (g_OverlayManager) g_OverlayManager->Render(pSwapChain);

        HRESULT hr = OriginalPresent(pSwapChain, SyncInterval, Flags);
        g_PacingController->UpdateDisplay();

        pContext->Release();
        pDevice->Release();
        return hr;
    }

    bool Initialize() {
        if (MH_Initialize() != MH_OK) {}
        ID3D11Device* pDevice = nullptr; ID3D11DeviceContext* pContext = nullptr; IDXGISwapChain* pSwapChain = nullptr;
        D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
        DXGI_SWAP_CHAIN_DESC scd = { {0, 0, {0, 0}, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED, DXGI_MODE_SCALING_UNSPECIFIED}, {1, 0}, DXGI_USAGE_RENDER_TARGET_OUTPUT, 1, GetDesktopWindow(), TRUE, DXGI_SWAP_EFFECT_DISCARD, 0 };
        if (FAILED(D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, &featureLevel, 1, D3D11_SDK_VERSION, &scd, &pSwapChain, &pDevice, nullptr, &pContext))) return false;

        void** vtable = *reinterpret_cast<void***>(pSwapChain);
        MH_CreateHook(vtable[8], &HookedPresent, reinterpret_cast<LPVOID*>(&OriginalPresent));
        MH_CreateHook(vtable[13], &HookedResizeBuffers, reinterpret_cast<LPVOID*>(&OriginalResizeBuffers));
        MH_EnableHook(MH_ALL_HOOKS);

        pSwapChain->Release(); pDevice->Release(); pContext->Release();
        return true;
    }

    void Shutdown() {
        MH_DisableHook(MH_ALL_HOOKS);
    }
}
