#include "frame_capture.h"
#include <iostream>

namespace FrameForge::Engine {

    FrameCapture::FrameCapture() {}
    FrameCapture::~FrameCapture() {}

    bool FrameCapture::Capture(IDXGISwapChain* pSwapChain) {
        if (!m_pDevice) {
            if (FAILED(pSwapChain->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(m_pDevice.GetAddressOf())))) {
                return false;
            }
            m_pDevice->GetImmediateContext(m_pContext.GetAddressOf());
        }

        Microsoft::WRL::ComPtr<ID3D11Texture2D> pBackBuffer;
        if (FAILED(pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(pBackBuffer.GetAddressOf())))) {
            return false;
        }

        if (!m_currentFrame) {
            if (!InitResources(pBackBuffer.Get())) return false;
        }

        // Rotate buffers: current becomes previous
        m_pContext->CopyResource(m_previousFrame.Get(), m_currentFrame.Get());
        
        // Capture new frame
        m_pContext->CopyResource(m_currentFrame.Get(), pBackBuffer.Get());

        return true;
    }

    bool FrameCapture::InitResources(ID3D11Texture2D* pBackBuffer) {
        D3D11_TEXTURE2D_DESC desc;
        pBackBuffer->GetDesc(&desc);

        // We want a texture we can copy to, but also read from in shaders later
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;

        if (FAILED(m_pDevice->CreateTexture2D(&desc, nullptr, m_currentFrame.GetAddressOf()))) {
            return false;
        }

        if (FAILED(m_pDevice->CreateTexture2D(&desc, nullptr, m_previousFrame.GetAddressOf()))) {
            return false;
        }

        std::cout << "[FrameForge] FrameCapture resources initialized: " << desc.Width << "x" << desc.Height << std::endl;
        return true;
    }

    ID3D11Texture2D* FrameCapture::CreateOutputTexture() {
        if (!m_currentFrame) return nullptr;

        D3D11_TEXTURE2D_DESC desc;
        m_currentFrame->GetDesc(&desc);
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;

        ID3D11Texture2D* pOutput = nullptr;
        m_pDevice->CreateTexture2D(&desc, nullptr, &pOutput);
        return pOutput;
    }

    ID3D11Texture2D* FrameCapture::CreateMotionVectorTexture() {
        if (!m_currentFrame) return nullptr;

        D3D11_TEXTURE2D_DESC desc;
        m_currentFrame->GetDesc(&desc);
        desc.Format = DXGI_FORMAT_R16G16_FLOAT; // Store 2D vectors
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;

        ID3D11Texture2D* pMV = nullptr;
        m_pDevice->CreateTexture2D(&desc, nullptr, &pMV);
        return pMV;
    }
}
