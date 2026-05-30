#include <windows.h>
#include <iostream>
#include "hooks/dx11_hook.h"

DWORD WINAPI MainThread(LPVOID lpParam) {
    // Create console for debugging
    AllocConsole();
    FILE* f;
    freopen_s(&f, "CONOUT$", "w", stdout);

    std::cout << "[FrameForge] Runtime Injected." << std::endl;

    if (FrameForge::Hooks::Initialize()) {
        std::cout << "[FrameForge] Hooks Initialized Successfully." << std::endl;
    } else {
        std::cerr << "[FrameForge] Failed to Initialize Hooks." << std::endl;
    }

    return 0;
}

BOOL WINAPI DllMain(HINSTANCE hModule, DWORD dwReason, LPVOID lpReserved) {
    switch (dwReason) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, MainThread, hModule, 0, nullptr);
        break;
    case DLL_PROCESS_DETACH:
        FrameForge::Hooks::Shutdown();
        break;
    }
    return TRUE;
}
