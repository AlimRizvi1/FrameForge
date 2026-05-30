#include "overlay.h"
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include <iostream>

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace FrameForge::Overlay {

    Manager::Manager() {}
    Manager::~Manager() { Shutdown(); }

    bool Manager::Initialize(IDXGISwapChain* pSwapChain) {
        if (m_initialized) return true;

        if (FAILED(pSwapChain->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(&m_pDevice)))) {
            return false;
        }

        m_pDevice->GetImmediateContext(&m_pContext);

        DXGI_SWAP_CHAIN_DESC sd;
        pSwapChain->GetDesc(&sd);
        InitImGui(sd.OutputWindow);

        ID3D11Texture2D* pBackBuffer = nullptr;
        pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pBackBuffer));
        m_pDevice->CreateRenderTargetView(pBackBuffer, nullptr, &m_pRenderTargetView);
        pBackBuffer->Release();

        m_initialized = true;
        std::cout << "[FrameForge] Overlay Initialized." << std::endl;
        return true;
    }

    void Manager::InitImGui(HWND hwnd) {
        ImGui::CreateContext();
        ImGui_ImplWin32_Init(hwnd);
        ImGui_ImplDX11_Init(m_pDevice, m_pContext);
        ImGui::StyleColorsDark();
    }

    void Manager::Render() {
        if (!m_initialized) return;

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(200, 100), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("FrameForge Status", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) {
            ImGui::Text("Render FPS: %.1f", ImGui::GetIO().Framerate);
            ImGui::Text("State: Active");
        }
        ImGui::End();

        ImGui::Render();
        m_pContext->OMSetRenderTargets(1, &m_pRenderTargetView, nullptr);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    }

    void Manager::Shutdown() {
        if (!m_initialized) return;

        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();

        if (m_pRenderTargetView) m_pRenderTargetView->Release();
        if (m_pContext) m_pContext->Release();
        if (m_pDevice) m_pDevice->Release();

        m_initialized = false;
    }
}
