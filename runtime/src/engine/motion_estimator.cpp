#include "motion_estimator.h"
#include <d3dcompiler.h>
#include <iostream>

namespace FrameForge::Engine {

    MotionEstimator::MotionEstimator() {}
    MotionEstimator::~MotionEstimator() {}

    bool MotionEstimator::Initialize(ID3D11Device* pDevice) {
        m_pDevice = pDevice;
        return CreateMotionShader();
    }

    bool MotionEstimator::CreateMotionShader() {
        const char* shaderSrc = R"(
            Texture2D<float4> prevFrame : register(t0);
            Texture2D<float4> currFrame : register(t1);
            RWTexture2D<float2> motionVectors : register(u0);

            [numthreads(16, 16, 1)]
            void main(uint3 dtid : SV_DispatchThreadID) {
                // Extremely simplified block matching for MVP
                // In reality, this would search a window
                float2 bestVec = float2(0, 0);
                float minDiff = 1000000.0;

                float4 currColor = currFrame[dtid.xy];

                for(int y = -4; y <= 4; y++) {
                    for(int x = -4; x <= 4; x++) {
                        float4 prevColor = prevFrame[dtid.xy + int2(x, y)];
                        float diff = distance(currColor, prevColor);
                        if(diff < minDiff) {
                            minDiff = diff;
                            bestVec = float2(x, y);
                        }
                    }
                }

                motionVectors[dtid.xy] = bestVec;
            }
        )";

        Microsoft::WRL::ComPtr<ID3DBlob> shaderBlob;
        Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;

        HRESULT hr = D3DCompile(shaderSrc, strlen(shaderSrc), nullptr, nullptr, nullptr, "main", "cs_5_0", 0, 0, &shaderBlob, &errorBlob);
        if (FAILED(hr)) {
            if (errorBlob) {
                std::cerr << "[FrameForge] Motion Shader compile error: " << (char*)errorBlob->GetBufferPointer() << std::endl;
            }
            return false;
        }

        hr = m_pDevice->CreateComputeShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, m_pMotionShader.GetAddressOf());
        return SUCCEEDED(hr);
    }

    void MotionEstimator::Estimate(ID3D11DeviceContext* pContext, ID3D11Texture2D* pPrev, ID3D11Texture2D* pCurr, ID3D11Texture2D* pMotionVectors) {
        if (!m_pMotionShader) return;

        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srvPrev, srvCurr;
        Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> uavMotion;

        m_pDevice->CreateShaderResourceView(pPrev, nullptr, srvPrev.GetAddressOf());
        m_pDevice->CreateShaderResourceView(pCurr, nullptr, srvCurr.GetAddressOf());
        m_pDevice->CreateUnorderedAccessView(pMotionVectors, nullptr, uavMotion.GetAddressOf());

        pContext->CSSetShader(m_pMotionShader.Get(), nullptr, 0);
        ID3D11ShaderResourceView* srvs[] = { srvPrev.Get(), srvCurr.Get() };
        pContext->CSSetShaderResources(0, 2, srvs);
        pContext->CSSetUnorderedAccessViews(0, 1, uavMotion.GetAddressOf(), nullptr);

        D3D11_TEXTURE2D_DESC desc;
        pMotionVectors->GetDesc(&desc);
        pContext->Dispatch((desc.Width + 15) / 16, (desc.Height + 15) / 16, 1);

        ID3D11ShaderResourceView* nullSRVs[] = { nullptr, nullptr };
        pContext->CSSetShaderResources(0, 2, nullSRVs);
        ID3D11UnorderedAccessView* nullUAVs[] = { nullptr };
        pContext->CSSetUnorderedAccessViews(0, 1, nullUAVs, nullptr);
    }
}
