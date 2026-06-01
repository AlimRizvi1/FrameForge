#include <windows.h>
#include <iostream>
#include <fstream>
#include <atomic>
#include <MinHook.h>
#include "hooks/dx11_hook.h"

static const char* FrameForgeLogPath = "F:\\FrameForgeRuntime.log";
static std::atomic<bool> g_HooksInitialized = false;

void LogToFile(const char* msg) {
    FILE* f = nullptr;
    if (fopen_s(&f, FrameForgeLogPath, "a") == 0 && f) {
        SYSTEMTIME st;
        GetLocalTime(&st);
        fprintf(f, "[%02d:%02d:%02d.%03d] %s\n", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, msg);
        fclose(f);
    }
}

void LogToConsole(const char* msg) {
    std::cout << msg << std::endl;
    LogToFile(msg);
}

DWORD WINAPI MainThread(LPVOID lpParam) {
    AllocConsole();
    FILE* f;
    freopen_s(&f, "CONOUT$", "w", stdout);

    LogToConsole("[FrameForge] Runtime Injected. Worker thread starting...");

    const int MAX_RETRIES = 120; // 2 minutes
    int retries = 0;

    while (retries < MAX_RETRIES && !g_HooksInitialized.load()) {
        HMODULE d3d11 = GetModuleHandleA("d3d11.dll");
        
        if (d3d11) {
            LogToConsole("[FrameForge] d3d11.dll detected. Starting discovery...");
            if (FrameForge::Hooks::Initialize()) {
                LogToConsole("[FrameForge] Hooks Initialized Successfully.");
                g_HooksInitialized.store(true);
                break;
            } else {
                LogToConsole("[FrameForge] Hooks::Initialize failed. Retrying...");
            }
        }

        retries++;
        if (retries % 10 == 0) {
            char buf[128];
            sprintf_s(buf, sizeof(buf), "[FrameForge] Waiting for d3d11.dll... (%d/%d)", retries, MAX_RETRIES);
            LogToConsole(buf);
        }
        Sleep(1000);
    }

    if (g_HooksInitialized.load()) {
        LogToConsole("[FrameForge] Engine ACTIVE. Entering heartbeat mode.");
        // Keep thread alive to monitor status
        while (true) {
            Sleep(5000);
            LogToFile("[FrameForge] Heartbeat - Engine Still Injected");
        }
    } else {
        LogToConsole("[FrameForge] FAILED to initialize within timeout.");
    }

    return 0;
}

BOOL WINAPI DllMain(HINSTANCE hModule, DWORD dwReason, LPVOID lpReserved) {
    switch (dwReason) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        LogToFile("[FrameForge] DllMain ATTACH");
        if (!CreateThread(nullptr, 0, MainThread, hModule, 0, nullptr)) {
            LogToFile("[FrameForge] FATAL: Failed to create MainThread");
            return FALSE;
        }
        break;
    case DLL_PROCESS_DETACH:
        LogToFile("[FrameForge] DllMain DETACH - DLL is being unloaded!");
        FrameForge::Hooks::Shutdown();
        MH_Uninitialize();
        break;
    }
    return TRUE;
}
