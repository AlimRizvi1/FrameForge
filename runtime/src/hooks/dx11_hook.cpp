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
    typedef HRESULT(STDMETHODCALLTYPE* ResizeBuffers)(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);
    ResizeBuffers OriginalResizeBuffers = nullptr;

    typedef HRESULT(STDMETHODCALLTYPE* D3D11CreateDeviceAndSwapChain_t)(
        IDXGIAdapter* pAdapter,
        D3D_DRIVER_TYPE DriverType,
        HMODULE Software,
        UINT Flags,
        const D3D_FEATURE_LEVEL* pFeatureLevels,
        UINT FeatureLevels,
        UINT SDKVersion,
        const DXGI_SWAP_CHAIN_DESC* pSwapChainDesc,
        IDXGISwapChain** ppSwapChain,
        ID3D11Device** ppDevice,
        D3D_FEATURE_LEVEL* pFeatureLevel,
        ID3D11DeviceContext** ppImmediateContext
    );
    D3D11CreateDeviceAndSwapChain_t OriginalD3D11CreateDeviceAndSwapChain = nullptr;

    FrameForge::Engine::FrameCapture* g_FrameCapture = nullptr;
    FrameForge::Engine::PacingController* g_PacingController = nullptr;
    FrameForge::Engine::InterpolationEngine* g_InterpolationEngine = nullptr;
    FrameForge::Engine::MotionEstimator* g_MotionEstimator = nullptr;
    FrameForge::Overlay::Manager* g_OverlayManager = nullptr;
    
    ID3D11Texture2D* g_InterpolatedFrame = nullptr;
    ID3D11Texture2D* g_MotionVectors = nullptr;
    HWND g_hWindow = nullptr;
    WNDPROC g_OriginalWndProc = nullptr;

    std::atomic<bool> g_SwapchainHooked{ false };

    LRESULT CALLBACK HookedWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
            return true;
        return CallWindowProc(g_OriginalWndProc, hWnd, msg, wParam, lParam);
    }

    HRESULT STDMETHODCALLTYPE HookedPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags) {
        // Initialize all components on first call
        if (!g_FrameCapture) {
            g_FrameCapture = new FrameForge::Engine::FrameCapture();
            std::cout << "[FrameForge] FrameCapture initialized." << std::endl;
        }

        if (!g_PacingController) {
            g_PacingController = new FrameForge::Engine::PacingController();
            g_PacingController->SetTargetFPS(60); // Default
            std::cout << "[FrameForge] PacingController initialized." << std::endl;
        }

        if (!g_InterpolationEngine) {
            ID3D11Device* pDevice = nullptr;
            pSwapChain->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(&pDevice));
            g_InterpolationEngine = new FrameForge::Engine::InterpolationEngine();
            if (g_InterpolationEngine->Initialize(pDevice)) {
                std::cout << "[FrameForge] InterpolationEngine initialized." << std::endl;
            } else {
                std::cerr << "[FrameForge] Failed to initialize InterpolationEngine." << std::endl;
            }
            pDevice->Release();
        }

        if (!g_MotionEstimator) {
            ID3D11Device* pDevice = nullptr;
            pSwapChain->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(&pDevice));
            g_MotionEstimator = new FrameForge::Engine::MotionEstimator();
            if (g_MotionEstimator->Initialize(pDevice)) {
                std::cout << "[FrameForge] MotionEstimator initialized." << std::endl;
            } else {
                std::cerr << "[FrameForge] Failed to initialize MotionEstimator." << std::endl;
            }
            pDevice->Release();
        }

        if (!g_OverlayManager) {
            g_OverlayManager = new FrameForge::Overlay::Manager();
            if (g_OverlayManager->Initialize(pSwapChain)) {
                std::cout << "[FrameForge] Overlay initialized." << std::endl;
            } else {
                std::cerr << "[FrameForge] Failed to initialize Overlay." << std::endl;
            }

            DXGI_SWAP_CHAIN_DESC sd;
            pSwapChain->GetDesc(&sd);
            g_hWindow = sd.OutputWindow;
            g_OriginalWndProc = reinterpret_cast<WNDPROC>(SetWindowLongPtr(g_hWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(HookedWndProc)));
        }

        // Track render frame
        g_PacingController->UpdateRender();
        
        // Capture frames for motion estimation
        g_FrameCapture->Capture(pSwapChain);

        // Process interpolation if we have a previous frame
        if (g_FrameCapture->GetPreviousFrame()) {
            if (!g_InterpolatedFrame) {
                g_InterpolatedFrame = g_FrameCapture->CreateOutputTexture();
                std::cout << "[FrameForge] Interpolated frame texture created." << std::endl;
            }
            if (!g_MotionVectors) {
                g_MotionVectors = g_FrameCapture->CreateMotionVectorTexture();
                std::cout << "[FrameForge] Motion vector texture created." << std::endl;
            }

            ID3D11Device* pDevice = nullptr;
            pSwapChain->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(&pDevice));
            ID3D11DeviceContext* pContext = nullptr;
            pDevice->GetImmediateContext(&pContext);

            // 1. Estimate Motion
            g_MotionEstimator->Estimate(pContext, 
                g_FrameCapture->GetPreviousFrame(), 
                g_FrameCapture->GetCurrentFrame(), 
                g_MotionVectors);

            // 2. Interpolate with blend factor (0.5 = midpoint between frames)
            g_InterpolationEngine->Process(pContext, 
                g_FrameCapture->GetPreviousFrame(), 
                g_FrameCapture->GetCurrentFrame(), 
                g_InterpolatedFrame, 0.5f);

            // Copy interpolated frame to backbuffer to present it
            ID3D11Texture2D* pBackBuffer = nullptr;
            if (SUCCEEDED(pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pBackBuffer)))) {
                pContext->CopyResource(pBackBuffer, g_InterpolatedFrame);
                pBackBuffer->Release();
            }

            pContext->Release();
            pDevice->Release();
        }

        // Track display frame
        g_PacingController->UpdateDisplay();
        
        // Render overlay on top of the frame
        if (g_OverlayManager) {
            g_OverlayManager->Render(pSwapChain);
        }

        return OriginalPresent ? OriginalPresent(pSwapChain, SyncInterval, Flags) : S_OK;
    }

    // ResizeBuffers hook: release RTV and mark for recreate
    HRESULT STDMETHODCALLTYPE HookedResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags) {
        std::cout << "[FrameForge] HookedResizeBuffers called." << std::endl;
        // Release overlay resources and reinitialize on next Present.
        if (g_OverlayManager) {
            g_OverlayManager->Shutdown();
            delete g_OverlayManager;
            g_OverlayManager = nullptr;
        }

        HRESULT hr = OriginalResizeBuffers ? OriginalResizeBuffers(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags) : S_OK;

        // After resize, we'll reinitialize render targets on next Present
        return hr;
    }

    // Helper: install Present/ResizeBuffers hooks for a real swapchain instance
    bool HookSwapchainVTable(IDXGISwapChain* pSwapChain) {
        if (!pSwapChain) return false;
        if (g_SwapchainHooked.load()) return true;

        void** vtable = *reinterpret_cast<void***>(pSwapChain);
        void* present_addr = vtable[8]; // Present index
        void* resize_addr = vtable[13]; // ResizeBuffers index

        if (MH_CreateHook(present_addr, &HookedPresent, reinterpret_cast<LPVOID*>(&OriginalPresent)) != MH_OK) {
            std::cerr << "[FrameForge] Failed to create Present hook." << std::endl;
            return false;
        }
        if (MH_EnableHook(present_addr) != MH_OK) {
            std::cerr << "[FrameForge] Failed to enable Present hook." << std::endl;
            return false;
        }

        if (MH_CreateHook(resize_addr, &HookedResizeBuffers, reinterpret_cast<LPVOID*>(&OriginalResizeBuffers)) != MH_OK) {
            std::cerr << "[FrameForge] Failed to create ResizeBuffers hook." << std::endl;
            // Try to continue with Present only
        } else {
            if (MH_EnableHook(resize_addr) != MH_OK) {
                std::cerr << "[FrameForge] Failed to enable ResizeBuffers hook." << std::endl;
            }
        }

        g_SwapchainHooked.store(true);
        std::cout << "[FrameForge] Swapchain vtable hooked (Present + ResizeBuffers)." << std::endl;
        return true;
    }

    // Hook for D3D11CreateDeviceAndSwapChain to capture the real swapchain safely
    HRESULT STDMETHODCALLTYPE HookedD3D11CreateDeviceAndSwapChain(
        IDXGIAdapter* pAdapter,
        D3D_DRIVER_TYPE DriverType,
        HMODULE Software,
        UINT Flags,
        const D3D_FEATURE_LEVEL* pFeatureLevels,
        UINT FeatureLevels,
        UINT SDKVersion,
        const DXGI_SWAP_CHAIN_DESC* pSwapChainDesc,
        IDXGISwapChain** ppSwapChain,
        ID3D11Device** ppDevice,
        D3D_FEATURE_LEVEL* pFeatureLevel,
        ID3D11DeviceContext** ppImmediateContext)
    {
        HRESULT hr = OriginalD3D11CreateDeviceAndSwapChain(
            pAdapter, DriverType, Software, Flags,
            pFeatureLevels, FeatureLevels, SDKVersion,
            pSwapChainDesc, ppSwapChain, ppDevice, pFeatureLevel, ppImmediateContext);

        if (SUCCEEDED(hr) && ppSwapChain && *ppSwapChain) {
            std::cout << "[FrameForge] D3D11CreateDeviceAndSwapChain succeeded - hooking swapchain." << std::endl;
            // Safe to install vtable hooks now
            HookSwapchainVTable(*ppSwapChain);
        }

        return hr;
    }

    bool Initialize() {
        // MinHook is initialized once by the runtime worker thread before this initialization.
        // Do not call MH_Initialize() again here; repeated initialization can fail.

        // Hook D3D11CreateDeviceAndSwapChain so we capture the engine's real swapchain
        HMODULE hD3D11 = GetModuleHandleA("d3d11.dll");
        if (!hD3D11) {
            // Not loaded yet - attempt to load the module so we can get the proc address
            hD3D11 = LoadLibraryA("d3d11.dll");
            if (!hD3D11) {
                std::cerr << "[FrameForge] d3d11.dll not available." << std::endl;
                // It's okay; we'll still succeed but hooks will be installed when module loads
            }
        }

        FARPROC proc = hD3D11 ? GetProcAddress(hD3D11, "D3D11CreateDeviceAndSwapChain") : nullptr;
        if (!proc) {
            // Could not find export right now; try to install a passive wait-and-hook by creating a small
            // polling thread that waits for the module and then hooks. For simplicity, return true and rely
            // on the worker thread to call Initialize again (MainThread does this once). Alternatively, you
            // could create a loader hook; keep this minimal here.
            std::cout << "[FrameForge] D3D11CreateDeviceAndSwapChain not found at init; will wait for runtime." << std::endl;
            return true;
        }

        OriginalD3D11CreateDeviceAndSwapChain = reinterpret_cast<D3D11CreateDeviceAndSwapChain_t>(proc);
        if (MH_CreateHook(OriginalD3D11CreateDeviceAndSwapChain, &HookedD3D11CreateDeviceAndSwapChain, reinterpret_cast<LPVOID*>(&OriginalD3D11CreateDeviceAndSwapChain)) != MH_OK) {
            std::cerr << "[FrameForge] Failed to create hook for D3D11CreateDeviceAndSwapChain." << std::endl;
            return false;
        }
        if (MH_EnableHook(OriginalD3D11CreateDeviceAndSwapChain) != MH_OK) {
            std::cerr << "[FrameForge] Failed to enable hook for D3D11CreateDeviceAndSwapChain." << std::endl;
            return false;
        }

        std::cout << "[FrameForge] Hooked D3D11CreateDeviceAndSwapChain successfully." << std::endl;
        return true;
    }

    void Shutdown() {
        MH_DisableHook(MH_ALL_HOOKS);
    }
}
