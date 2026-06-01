#include "pacing_controller.h"
#include <iostream>
#include <numeric>
#include <deque>

namespace FrameForge::Engine {

    static std::deque<long long> g_FrameTimes;
    static const size_t MAX_SAMPLES = 60;

    PacingController::PacingController() {
        auto now = std::chrono::steady_clock::now();
        m_lastGameFrameTime = now;
        m_lastPresentedTime = now;
        m_fpsLastTime = now;
        SetTargetFPS(60);
    }

    PacingController::~PacingController() {}

    void PacingController::SetTargetFPS(int fps) {
        m_targetFps = fps;
        m_targetInterval = std::chrono::microseconds(1000000 / fps);
    }

    bool PacingController::ShouldInsertFrame(float& outWeight) {
        auto now = std::chrono::steady_clock::now();
        
        auto elapsedSinceGame = std::chrono::duration_cast<std::chrono::microseconds>(now - m_lastGameFrameTime);
        auto elapsedSinceLastPresent = std::chrono::duration_cast<std::chrono::microseconds>(now - m_lastPresentedTime);

        // Calculate moving average game frametime
        long long avgFrameTime = m_targetInterval.count() * 2; // Default to 2x target (30fps if target 60)
        if (!g_FrameTimes.empty()) {
            avgFrameTime = std::accumulate(g_FrameTimes.begin(), g_FrameTimes.end(), 0LL) / g_FrameTimes.size();
        }

        // DYNAMIC PACING LOGIC:
        // We want to insert a frame if we are roughly halfway through the EXPECTED game frametime,
        // provided that enough time has passed since our last presentation to not exceed our target refresh.
        
        long long triggerPoint = avgFrameTime / 2;
        
        if (elapsedSinceGame >= std::chrono::microseconds(triggerPoint) && 
            elapsedSinceLastPresent >= m_targetInterval &&
            elapsedSinceGame < std::chrono::microseconds(avgFrameTime)) 
        {
            outWeight = (float)elapsedSinceGame.count() / (float)avgFrameTime;
            if (outWeight > 0.95f) outWeight = 0.95f;

            m_lastPresentedTime = now;
            return true;
        }

        return false;
    }

    void PacingController::UpdateRender() {
        auto now = std::chrono::steady_clock::now();
        
        // Record frametime sample
        long long frameTime = std::chrono::duration_cast<std::chrono::microseconds>(now - m_lastGameFrameTime).count();
        if (frameTime > 0 && frameTime < 500000) { // Sanity check
            g_FrameTimes.push_back(frameTime);
            if (g_FrameTimes.size() > MAX_SAMPLES) g_FrameTimes.pop_front();
        }

        m_renderCount++;
        m_lastGameFrameTime = now;
        m_lastPresentedTime = now;

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_fpsLastTime);
        if (elapsed.count() >= 1000) {
            m_renderFps = static_cast<float>(m_renderCount) * 1000.0f / elapsed.count();
            m_displayFps = static_cast<float>(m_displayCount) * 1000.0f / elapsed.count();
            m_renderCount = 0;
            m_displayCount = 0;
            m_fpsLastTime = now;
        }
    }

    void PacingController::UpdateDisplay() {
        m_displayCount++;
    }
}
