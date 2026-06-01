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

static bool IsHelperProcess() {
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    std::string name(path);
    size_t lastSlash = name.find_last_of("\\/");
    if (lastSlash != std::string::npos) name = name.substr(lastSlash + 1);
    for (auto& c : name) c = tolower(c);

    const char* blacklist[] = { "crash", "report", "cef", "helper", "service", "overlay", "steam", "epic", "battle", "unity", "unrealcef", "socialclub" };
    for (auto b : blacklist) {
        if (name.find(b) != std::string::npos) return true;
    }
    return false;
}

DWORD WINAPI MainThread(LPVOID lpParam) {
    LogToFile("Runtime Injected. Worker thread starting...");

    if (IsHelperProcess()) {
        LogToFile("Detected helper process. Terminating worker thread.");
        return 0;
    }

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
        LogToFile("Engine ACTIVE. Entering long-interval heartbeat mode.");
        while (true) {
            Sleep(60000); // 1 minute heartbeat
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
