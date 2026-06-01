#include "dx11_hook.h"
#include <MinHook.h>
#include <iostream>
#include <vector>
#include "../engine/frame_capture.h"
#include "../engine/pacing_controller.h"
#include "../engine/interpolation_engine.h"
#include "../engine/motion_estimator.h"
#include "../overlay/overlay.h"

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace FrameForge::Hooks {

    typedef HRESULT(STDMETHODCALLTYPE* Present)(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
    Present OriginalPresent = nullptr;

    FrameForge::Engine::FrameCapture* g_FrameCapture = nullptr;
    FrameForge::Engine::PacingController* g_PacingController = nullptr;
    FrameForge::Engine::InterpolationEngine* g_InterpolationEngine = nullptr;
    FrameForge::Engine::MotionEstimator* g_MotionEstimator = nullptr;
    FrameForge::Overlay::Manager* g_OverlayManager = nullptr;
    
    ID3D11Texture2D* g_InterpolatedFrame = nullptr;
    ID3D11Texture2D* g_MotionVectors = nullptr;
    HWND g_hWindow = nullptr;
    WNDPROC g_OriginalWndProc = nullptr;

    LRESULT CALLBACK HookedWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
            return true;
        return CallWindowProc(g_OriginalWndProc, hWnd, msg, wParam, lParam);
    }

    HRESULT STDMETHODCALLTYPE HookedPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags) {
        if (!g_FrameCapture) g_FrameCapture = new FrameForge::Engine::FrameCapture();
        if (!g_PacingController) g_PacingController = new FrameForge::Engine::PacingController();

        if (!g_InterpolationEngine) {
            ID3D11Device* pDevice = nullptr;
            pSwapChain->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(&pDevice));
            g_InterpolationEngine = new FrameForge::Engine::InterpolationEngine();
            g_InterpolationEngine->Initialize(pDevice);
            pDevice->Release();
        }

        if (!g_MotionEstimator) {
            ID3D11Device* pDevice = nullptr;
            pSwapChain->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(&pDevice));
            g_MotionEstimator = new FrameForge::Engine::MotionEstimator();
            g_MotionEstimator->Initialize(pDevice);
            pDevice->Release();
        }

        if (!g_OverlayManager) {
            g_OverlayManager = new FrameForge::Overlay::Manager();
            if (g_OverlayManager->Initialize(pSwapChain)) {
                DXGI_SWAP_CHAIN_DESC sd;
                pSwapChain->GetDesc(&sd);
                g_hWindow = sd.OutputWindow;
                g_OriginalWndProc = reinterpret_cast<WNDPROC>(SetWindowLongPtr(g_hWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(HookedWndProc)));
            }
        }

        g_PacingController->UpdateRender();
        g_FrameCapture->Capture(pSwapChain);

        if (g_FrameCapture->GetPreviousFrame()) {
            if (!g_InterpolatedFrame) g_InterpolatedFrame = g_FrameCapture->CreateOutputTexture();
            if (!g_MotionVectors) g_MotionVectors = g_FrameCapture->CreateMotionVectorTexture();

            ID3D11Device* pDevice = nullptr;
            pSwapChain->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(&pDevice));
            ID3D11DeviceContext* pContext = nullptr;
            pDevice->GetImmediateContext(&pContext);

            g_MotionEstimator->Estimate(pContext, g_FrameCapture->GetPreviousFrame(), g_FrameCapture->GetCurrentFrame(), g_MotionVectors);
            g_InterpolationEngine->Process(pContext, g_FrameCapture->GetPreviousFrame(), g_FrameCapture->GetCurrentFrame(), g_InterpolatedFrame, 0.5f);

            ID3D11Texture2D* pBackBuffer = nullptr;
            if (SUCCEEDED(pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pBackBuffer)))) {
                pContext->CopyResource(pBackBuffer, g_InterpolatedFrame);
                pBackBuffer->Release();
            }

            pContext->Release();
            pDevice->Release();
        }

        g_PacingController->UpdateDisplay();
        if (g_OverlayManager) g_OverlayManager->Render(pSwapChain);

        return OriginalPresent(pSwapChain, SyncInterval, Flags);
    }

    bool Initialize() {
        if (MH_Initialize() != MH_OK) return false;

        // Use dummy device to find Present VTable address
        // This works whether the game has already initialized DX11 or not
        ID3D11Device* pDevice = nullptr;
        ID3D11DeviceContext* pContext = nullptr;
        IDXGISwapChain* pSwapChain = nullptr;

        D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
        DXGI_SWAP_CHAIN_DESC scd = {};
        scd.BufferCount = 1;
        scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        scd.OutputWindow = GetDesktopWindow(); // Dummy window
        scd.SampleDesc.Count = 1;
        scd.Windowed = TRUE;
        scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

        HRESULT hr = D3D11CreateDeviceAndSwapChain(
            nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, &featureLevel, 1,
            D3D11_SDK_VERSION, &scd, &pSwapChain, &pDevice, nullptr, &pContext
        );

        if (FAILED(hr)) {
            std::cerr << "[FrameForge] Failed to create dummy device for discovery." << std::endl;
            return false;
        }

        void** vtable = *reinterpret_cast<void***>(pSwapChain);
        void* present_addr = vtable[8];

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

        std::cout << "[FrameForge] Hooking Discovery Successful. Target: " << present_addr << std::endl;
        return true;
    }

    void Shutdown() {
        MH_DisableHook(MH_ALL_HOOKS);
    }
}
