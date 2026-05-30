#include "dx11_hook.h"
#include <MinHook.h>
#include <iostream>
#include <vector>

namespace FrameForge::Hooks {

    typedef HRESULT(STDMETHODCALLTYPE* Present)(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
    Present OriginalPresent = nullptr;

    HRESULT STDMETHODCALLTYPE HookedPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags) {
        static bool initial_log = false;
        if (!initial_log) {
            std::cout << "[FrameForge] Present Called!" << std::endl;
            initial_log = true;
        }

        // TODO: Frame Capture & Interpolation logic here

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
