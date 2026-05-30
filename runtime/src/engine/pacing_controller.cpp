#include "pacing_controller.h"

namespace FrameForge::Engine {

    PacingController::PacingController() {
        m_lastPresentTime = std::chrono::steady_clock::now();
        m_fpsLastTime = m_lastPresentTime;
        SetTargetFPS(60);
    }

    PacingController::~PacingController() {}

    void PacingController::SetTargetFPS(int fps) {
        m_targetFps = fps;
        m_targetInterval = std::chrono::microseconds(1000000 / fps);
    }

    bool PacingController::ShouldPresent() {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - m_lastPresentTime);

        if (elapsed >= m_targetInterval) {
            m_lastPresentTime = now;
            return true;
        }

        return false;
    }

    void PacingController::Update() {
        m_frameCount++;
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_fpsLastTime);

        if (elapsed.count() >= 1000) {
            m_actualFps = static_cast<float>(m_frameCount) * 1000.0f / elapsed.count();
            m_frameCount = 0;
            m_fpsLastTime = now;
        }
    }
}
