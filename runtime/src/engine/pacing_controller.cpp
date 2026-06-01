#include "pacing_controller.h"
#include <iostream>

namespace FrameForge::Engine {

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
        
        // Time since the last time we actually showed ANYTHING on screen
        auto elapsedSincePresent = std::chrono::duration_cast<std::chrono::microseconds>(now - m_lastPresentedTime);
        
        // Time since the game itself gave us a frame
        auto elapsedSinceGame = std::chrono::duration_cast<std::chrono::microseconds>(now - m_lastGameFrameTime);

        // SMART BEAT: If we are at the halfway point of our target interval, 
        // AND the game hasn't given us a new frame yet, insert an interpolated one.
        if (elapsedSincePresent >= m_targetInterval && elapsedSinceGame < m_targetInterval * 2) {
            // Calculate dynamic weight based on the game's current rhythm
            // If the game is at 30fps (33ms) and we want 60fps (16ms), weight should be ~0.5
            outWeight = (float)elapsedSinceGame.count() / (m_targetInterval.count() * 2.0f);
            if (outWeight > 0.9f) outWeight = 0.9f; // Cap to avoid jumping
            
            m_lastPresentedTime = now;
            return true;
        }

        return false;
    }

    void PacingController::UpdateRender() {
        m_renderCount++;
        m_lastGameFrameTime = std::chrono::steady_clock::now();
        m_lastPresentedTime = m_lastGameFrameTime; // Game frame counts as a presentation

        auto now = std::chrono::steady_clock::now();
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
