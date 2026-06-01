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
        fprintf(f, "[%02d:%02d:%02d.%03d] [%lu] %s\n", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, GetCurrentProcessId(), msg);
        fclose(f);
    }
}

DWORD WINAPI MainThread(LPVOID lpParam) {
    // Only allocate console if a specific debug file exists, to keep game startup clean
    if (GetFileAttributesA("F:\\FrameForgeDebug.txt") != INVALID_FILE_ATTRIBUTES) {
        AllocConsole();
        FILE* f;
        freopen_s(&f, "CONOUT$", "w", stdout);
    }

    LogToFile("Runtime Injected. Worker thread starting...");

    const int MAX_RETRIES = 60; 
    int retries = 0;

    while (retries < MAX_RETRIES && !g_HooksInitialized.load()) {
        HMODULE d3d11 = GetModuleHandleA("d3d11.dll");
        
        if (d3d11) {
            LogToFile("d3d11.dll detected. Starting discovery...");
            if (FrameForge::Hooks::Initialize()) {
                LogToFile("Hooks Initialized Successfully.");
                g_HooksInitialized.store(true);
                break;
            } else {
                LogToFile("Hooks::Initialize failed. Retrying...");
            }
        }

        retries++;
        Sleep(1000);
    }

    if (g_HooksInitialized.load()) {
        LogToFile("Engine ACTIVE. Entering heartbeat mode.");
        while (true) {
            Sleep(10000);
            LogToFile("Heartbeat - Engine Still Injected");
        }
    } else {
        LogToFile("FAILED to initialize within timeout.");
    }

    return 0;
}

BOOL WINAPI DllMain(HINSTANCE hModule, DWORD dwReason, LPVOID lpReserved) {
    switch (dwReason) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        LogToFile("DllMain ATTACH");
        if (!CreateThread(nullptr, 0, MainThread, hModule, 0, nullptr)) {
            LogToFile("FATAL: Failed to create MainThread");
            return FALSE;
        }
        break;
    case DLL_PROCESS_DETACH:
        LogToFile("DllMain DETACH - DLL is being unloaded!");
        FrameForge::Hooks::Shutdown();
        MH_Uninitialize();
        break;
    }
    return TRUE;
}
