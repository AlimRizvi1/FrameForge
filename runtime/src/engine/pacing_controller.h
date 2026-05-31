#pragma once
#include <windows.h>
#include <chrono>

namespace FrameForge::Engine {
    class PacingController {
    public:
        PacingController();
        ~PacingController();

        void SetTargetFPS(int fps);
        bool ShouldPresent();
        void UpdateRender(); // Called when game renders a frame
        void UpdateDisplay(); // Called when FrameForge presents a frame (real or interp)

        float GetRenderFPS() const { return m_renderFps; }
        float GetDisplayFPS() const { return m_displayFps; }

    private:
        int m_targetFps = 60;
        std::chrono::steady_clock::time_point m_lastPresentTime;
        std::chrono::microseconds m_targetInterval;

        int m_renderCount = 0;
        int m_displayCount = 0;
        std::chrono::steady_clock::time_point m_fpsLastTime;
        float m_renderFps = 0.0f;
        float m_displayFps = 0.0f;
    };
}
