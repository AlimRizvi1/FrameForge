#pragma once
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>

namespace FrameForge::Engine {
    class PacingController;
}

namespace FrameForge::Hooks {
    bool Initialize();
    void Shutdown();

    extern FrameForge::Engine::PacingController* g_PacingController;
}
