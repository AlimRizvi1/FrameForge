#include <windows.h>
#include <iostream>
#include <atomic>
#include <MinHook.h>
#include "hooks/dx11_hook.h"

// Global state
static std::atomic<bool> g_HooksInitialized = false;

void LogToConsole(const char* msg) {
    std::cout << msg << std::endl;
}

DWORD WINAPI MainThread(LPVOID lpParam) {
    // Create console for debugging
    AllocConsole();
    FILE* f;
    freopen_s(&f, "CONOUT$", "w", stdout);

    LogToConsole("[FrameForge] Runtime Injected. Monitoring for DirectX 11...");

    const int MAX_RETRIES = 60; // 60 seconds
    int retries = 0;

    while (retries < MAX_RETRIES && !g_HooksInitialized.load()) {
        HMODULE d3d11 = GetModuleHandleA("d3d11.dll");
        
        if (d3d11) {
            LogToConsole("[FrameForge] d3d11.dll detected. Initializing hooks...");
            if (FrameForge::Hooks::Initialize()) {
                LogToConsole("[FrameForge] Hooks Initialized Successfully.");
                g_HooksInitialized.store(true);
                break;
            } else {
                LogToConsole("[FrameForge] Hook initialization attempt failed. Will retry...");
            }
        }

        retries++;
        if (retries % 5 == 0) {
            char buf[128];
            sprintf_s(buf, sizeof(buf), "[FrameForge] Still waiting for d3d11.dll... (attempt %d/%d)", retries, MAX_RETRIES);
            LogToConsole(buf);
        }
        
        Sleep(1000);
    }

    if (!g_HooksInitialized) {
        LogToConsole("[FrameForge] FAILED to initialize hooks within timeout.");
    } else {
        LogToConsole("[FrameForge] Engine is now ACTIVE.");
    }

    LogToConsole("[FrameForge] MainThread reaching end. DLL remains in memory.");
    return 0;
}

BOOL WINAPI DllMain(HINSTANCE hModule, DWORD dwReason, LPVOID lpReserved) {
    switch (dwReason) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        if (!CreateThread(nullptr, 0, MainThread, hModule, 0, nullptr)) {
            return FALSE;
        }
        break;
    case DLL_PROCESS_DETACH:
        LogToConsole("[FrameForge] Runtime Detaching...");
        FrameForge::Hooks::Shutdown();
        MH_Uninitialize();
        break;
    }
    return TRUE;
}
