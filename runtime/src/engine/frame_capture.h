#pragma once
#include <d3d11.h>
#include <wrl/client.h>
#include <vector>

namespace FrameForge::Engine {
    class FrameCapture {
    public:
        FrameCapture();
        ~FrameCapture();

        bool Capture(IDXGISwapChain* pSwapChain);
        ID3D11Texture2D* GetCurrentFrame() { return m_currentFrame.Get(); }
        ID3D11Texture2D* GetPreviousFrame() { return m_previousFrame.Get(); }

    private:
        Microsoft::WRL::ComPtr<ID3D11Device> m_pDevice;
        Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_pContext;
        Microsoft::WRL::ComPtr<ID3D11Texture2D> m_currentFrame;
        Microsoft::WRL::ComPtr<ID3D11Texture2D> m_previousFrame;

        bool InitResources(ID3D11Texture2D* pBackBuffer);
    };
}
