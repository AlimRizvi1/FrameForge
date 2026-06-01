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
            return false;
        }

        m_pDevice->GetImmediateContext(&m_pContext);

        DXGI_SWAP_CHAIN_DESC sd;
        pSwapChain->GetDesc(&sd);
        InitImGui(sd.OutputWindow);

        m_initialized = true;
        return true;
    }

    void Manager::InitImGui(HWND hwnd) {
        ImGui::CreateContext();
        ImGui_ImplWin32_Init(hwnd);
        ImGui_ImplDX11_Init(m_pDevice, m_pContext);
        ImGui::StyleColorsDark();
        
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
    }

    void Manager::Render(IDXGISwapChain* pSwapChain) {
        if (!m_initialized) return;

        // Only create/update RTV if needed (caching for stability)
        if (!m_pRenderTargetView) {
            ID3D11Texture2D* pBackBuffer = nullptr;
            if (SUCCEEDED(pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pBackBuffer)))) {
                m_pDevice->CreateRenderTargetView(pBackBuffer, nullptr, &m_pRenderTargetView);
                pBackBuffer->Release();
            }
        }

        if (!m_pRenderTargetView) return;

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(240, 110), ImGuiCond_Always);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.02f, 0.02f, 0.02f, 0.85f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.46f, 0.72f, 0.0f, 0.4f));

        if (ImGui::Begin("FF_OVERLAY", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_AlwaysAutoResize)) {
            float renderFps = 0.0f, displayFps = 0.0f;
            if (FrameForge::Hooks::g_PacingController) {
                renderFps = FrameForge::Hooks::g_PacingController->GetRenderFPS();
                displayFps = FrameForge::Hooks::g_PacingController->GetDisplayFPS();
            }

            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "GAME RENDER:");
            ImGui::SameLine(140);
            ImGui::Text("%.1f FPS", renderFps);

            ImGui::TextColored(ImVec4(0.46f, 0.72f, 0.0f, 1.0f), "FRAMEFORGE:");
            ImGui::SameLine(140);
            ImGui::Text("%.1f FPS", displayFps);

            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.4f, 0.4f, 0.4f, 1.0f), "SMOOTHING:");
            ImGui::SameLine(140);
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.8f, 1.0f), "ACTIVE");
        }
        ImGui::End();

        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar();

        ImGui::Render();
        m_pContext->OMSetRenderTargets(1, &m_pRenderTargetView, nullptr);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    }

    void Manager::Shutdown() {
        if (!m_initialized) return;
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        if (m_pRenderTargetView) { m_pRenderTargetView->Release(); m_pRenderTargetView = nullptr; }
        m_initialized = false;
    }
}
