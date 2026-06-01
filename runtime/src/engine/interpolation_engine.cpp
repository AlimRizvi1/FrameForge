#include "interpolation_engine.h"
#include <d3dcompiler.h>
#include <iostream>

namespace FrameForge::Engine {

    InterpolationEngine::InterpolationEngine() {}
    InterpolationEngine::~InterpolationEngine() {}

    bool InterpolationEngine::Initialize(ID3D11Device* pDevice) {
        m_pDevice = pDevice;
        return CreateBlendShader();
    }

    bool InterpolationEngine::CreateBlendShader() {
        const char* shaderSrc = R"(
            Texture2D<float4> prevFrame : register(t0);
            Texture2D<float4> currFrame : register(t1);
            Texture2D<float2> motionVectors : register(t2);
            RWTexture2D<float4> outputFrame : register(u0);

            SamplerState linearSampler : register(s0);

            cbuffer Params : register(b0) {
                float weight;
                float3 padding;
            };

            [numthreads(16, 16, 1)]
            void main(uint3 dtid : SV_DispatchThreadID) {
                float2 texSize;
                prevFrame.GetDimensions(texSize.x, texSize.y);
                float2 uv = float2(dtid.xy) / texSize;
                
                // Get motion vector for this pixel
                float2 mv = motionVectors[dtid.xy];
                float2 mv_uv = mv / texSize;

                // WARP: Sample previous frame forward and current frame backward
                // We use point sampling for MV but bilinear for frames to reduce flickering
                float4 p = prevFrame.SampleLevel(linearSampler, uv + (mv_uv * weight), 0);
                float4 c = currFrame.SampleLevel(linearSampler, uv - (mv_uv * (1.0 - weight)), 0);
                
                outputFrame[dtid.xy] = lerp(p, c, weight);
            }
        )";

        Microsoft::WRL::ComPtr<ID3DBlob> shaderBlob;
        Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;

        HRESULT hr = D3DCompile(shaderSrc, strlen(shaderSrc), nullptr, nullptr, nullptr, "main", "cs_5_0", 0, 0, &shaderBlob, &errorBlob);
        if (FAILED(hr)) return false;

        hr = m_pDevice->CreateComputeShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, m_pBlendShader.GetAddressOf());
        return SUCCEEDED(hr);
    }

    void InterpolationEngine::Process(ID3D11DeviceContext* pContext, ID3D11Texture2D* pPrev, ID3D11Texture2D* pCurr, ID3D11Texture2D* pMV, ID3D11Texture2D* pOutput, float weight) {
        if (!m_pBlendShader) return;

        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srvPrev, srvCurr, srvMV;
        Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> uavOut;

        m_pDevice->CreateShaderResourceView(pPrev, nullptr, srvPrev.GetAddressOf());
        m_pDevice->CreateShaderResourceView(pCurr, nullptr, srvCurr.GetAddressOf());
        m_pDevice->CreateShaderResourceView(pMV, nullptr, srvMV.GetAddressOf());
        m_pDevice->CreateUnorderedAccessView(pOutput, nullptr, uavOut.GetAddressOf());

        struct { float weight; float padding[3]; } params = { weight };
        Microsoft::WRL::ComPtr<ID3D11Buffer> cb;
        D3D11_BUFFER_DESC cbd = { 16, D3D11_USAGE_DEFAULT, D3D11_BIND_CONSTANT_BUFFER, 0, 0, 0 };
        m_pDevice->CreateBuffer(&cbd, nullptr, cb.GetAddressOf());
        pContext->UpdateSubresource(cb.Get(), 0, nullptr, &params, 0, 0);

        pContext->CSSetShader(m_pBlendShader.Get(), nullptr, 0);
        ID3D11ShaderResourceView* srvs[] = { srvPrev.Get(), srvCurr.Get(), srvMV.Get() };
        pContext->CSSetShaderResources(0, 3, srvs);
        pContext->CSSetUnorderedAccessViews(0, 1, uavOut.GetAddressOf(), nullptr);
        pContext->CSSetConstantBuffers(0, 1, cb.GetAddressOf());

        D3D11_TEXTURE2D_DESC desc; pOutput->GetDesc(&desc);
        pContext->Dispatch((desc.Width + 15) / 16, (desc.Height + 15) / 16, 1);

        ID3D11ShaderResourceView* nullSRVs[] = { nullptr, nullptr, nullptr };
        pContext->CSSetShaderResources(0, 3, nullSRVs);
        ID3D11UnorderedAccessView* nullUAVs[] = { nullptr };
        pContext->CSSetUnorderedAccessViews(0, 1, nullUAVs, nullptr);
    }
}
