#pragma once
#include <d3d11.h>
#include <wrl/client.h>

namespace FrameForge::Engine {
    class MotionEstimator {
    public:
        MotionEstimator();
        ~MotionEstimator();

        bool Initialize(ID3D11Device* pDevice);
        void Estimate(ID3D11DeviceContext* pContext, ID3D11Texture2D* pPrev, ID3D11Texture2D* pCurr, ID3D11Texture2D* pMotionVectors);

    private:
        Microsoft::WRL::ComPtr<ID3D11Device> m_pDevice;
        Microsoft::WRL::ComPtr<ID3D11ComputeShader> m_pMotionShader;

        bool CreateMotionShader();
    };
}
