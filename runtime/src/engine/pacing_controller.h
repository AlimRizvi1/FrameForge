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
        void Update();

        float GetActualFPS() const { return m_actualFps; }

    private:
        int m_targetFps = 60;
        std::chrono::steady_clock::time_point m_lastPresentTime;
        std::chrono::microseconds m_targetInterval;

        int m_frameCount = 0;
        std::chrono::steady_clock::time_point m_fpsLastTime;
        float m_actualFps = 0.0f;
    };
}
