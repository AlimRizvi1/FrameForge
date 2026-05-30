#pragma once
#include <d3d11.h>
#include <wrl/client.h>

namespace FrameForge::Engine {
    class InterpolationEngine {
    public:
        InterpolationEngine();
        ~InterpolationEngine();

        bool Initialize(ID3D11Device* pDevice);
        void Process(ID3D11DeviceContext* pContext, ID3D11Texture2D* pPrev, ID3D11Texture2D* pCurr, ID3D11Texture2D* pOutput, float weight);

    private:
        Microsoft::WRL::ComPtr<ID3D11Device> m_pDevice;
        Microsoft::WRL::ComPtr<ID3D11ComputeShader> m_pBlendShader;

        bool CreateBlendShader();
    };
}
