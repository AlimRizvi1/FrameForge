#include "overlay.h"
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include <iostream>
#include "../hooks/dx11_hook.h"
#include "../engine/pacing_controller.h"

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace FrameForge::Overlay {

    Manager::Manager() {}
    Manager::~Manager() { Shutdown(); }

    bool Manager::Initialize(IDXGISwapChain* pSwapChain) {
        if (m_initialized) return true;

        if (FAILED(pSwapChain->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(&m_pDevice)))) {
            std::cerr << "[FrameForge] Overlay failed to get DX11 device." << std::endl;
            return false;
        }

        m_pDevice->GetImmediateContext(&m_pContext);

        if (!m_pContext) {
            std::cerr << "[FrameForge] Overlay failed to get immediate device context." << std::endl;
            return false;
        }

        DXGI_SWAP_CHAIN_DESC sd;
        if (FAILED(pSwapChain->GetDesc(&sd))) {
            std::cerr << "[FrameForge] Overlay failed to get swapchain description." << std::endl;
            return false;
        }

        InitImGui(sd.OutputWindow);

        ID3D11Texture2D* pBackBuffer = nullptr;
        if (FAILED(pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pBackBuffer)))) {
            std::cerr << "[FrameForge] Overlay failed to get current backbuffer." << std::endl;
            return false;
        }

        if (FAILED(m_pDevice->CreateRenderTargetView(pBackBuffer, nullptr, &m_pRenderTargetView))) {
            std::cerr << "[FrameForge] Overlay failed to create render target view." << std::endl;
            pBackBuffer->Release();
            return false;
        }
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

    void Manager::Render(IDXGISwapChain* pSwapChain) {
        if (!m_initialized) return;

        // Refresh RenderTargetView each frame to ensure it points to current backbuffer
        if (m_pRenderTargetView) {
            m_pRenderTargetView->Release();
            m_pRenderTargetView = nullptr;
        }
        
        ID3D11Texture2D* pBackBuffer = nullptr;
        if (FAILED(pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pBackBuffer)))) {
            std::cerr << "[FrameForge] Overlay failed to get backbuffer for render target." << std::endl;
            return;
        }
        
        if (FAILED(m_pDevice->CreateRenderTargetView(pBackBuffer, nullptr, &m_pRenderTargetView))) {
            std::cerr << "[FrameForge] Overlay failed to create render target view for render." << std::endl;
            pBackBuffer->Release();
            return;
        }
        pBackBuffer->Release();

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(220, 120), ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.70f);

        if (ImGui::Begin("FrameForge Status", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize)) {
            float renderFps = 0.0f;
            float displayFps = 0.0f;
            if (FrameForge::Hooks::g_PacingController) {
                renderFps = FrameForge::Hooks::g_PacingController->GetRenderFPS();
                displayFps = FrameForge::Hooks::g_PacingController->GetDisplayFPS();
            }
            
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Real FPS:");
            ImGui::SameLine(120);
            ImGui::Text("%.1f", renderFps);

            ImGui::TextColored(ImVec4(0.46f, 0.72f, 0.0f, 1.0f), "FrameForge FPS:");
            ImGui::SameLine(120);
            ImGui::Text("%.1f", displayFps);

            ImGui::Separator();
            ImGui::Text("State:");
            ImGui::SameLine(120);
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Active");
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
