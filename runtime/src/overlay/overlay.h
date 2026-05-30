#pragma once
#include <d3d11.h>
#include <dxgi.h>

namespace FrameForge::Overlay {
    class Manager {
    public:
        Manager();
        ~Manager();

        bool Initialize(IDXGISwapChain* pSwapChain);
        void Render();
        void Shutdown();

    private:
        bool m_initialized = false;
        ID3D11Device* m_pDevice = nullptr;
        ID3D11DeviceContext* m_pContext = nullptr;
        ID3D11RenderTargetView* m_pRenderTargetView = nullptr;

        void InitImGui(HWND hwnd);
    };
}
