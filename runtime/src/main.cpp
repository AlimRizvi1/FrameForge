#include <windows.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <atomic>
#include <MinHook.h>
#include "hooks/dx11_hook.h"

static const char* FrameForgeLogPath = "F:\\FrameForgeRuntime.log";

// Simple file logger that avoids STL complexity
void LogToFile(const char* msg) {
    FILE* f = nullptr;
    if (fopen_s(&f, FrameForgeLogPath, "a") == 0 && f) {
        fprintf(f, "[%.0f] %s\n", GetTickCount64() / 1000.0, msg);
        fclose(f);
    }
}

void LogToConsole(const char* msg) {
    std::cout << msg << std::endl;
    LogToFile(msg);
}

// Global state for LoadLibrary hooking
static std::atomic<bool> g_HooksInitialized = false;
static std::atomic<bool> g_HookActivationRequested = false;

typedef struct _UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PWCH Buffer;
} UNICODE_STRING, *PUNICODE_STRING;
typedef const UNICODE_STRING* PCUNICODE_STRING;

typedef enum _LDR_DLL_NOTIFICATION_REASON {
    LdrDllNotificationReasonLoaded = 1,
    LdrDllNotificationReasonUnloaded = 2
} LDR_DLL_NOTIFICATION_REASON;

typedef struct _LDR_DLL_LOADED_NOTIFICATION_DATA {
    ULONG Flags;
    PCUNICODE_STRING FullDllName;
    PCUNICODE_STRING BaseDllName;
    PVOID DllBase;
    ULONG SizeOfImage;
} LDR_DLL_LOADED_NOTIFICATION_DATA, *PLDR_DLL_LOADED_NOTIFICATION_DATA;

typedef union _LDR_DLL_NOTIFICATION_DATA {
    LDR_DLL_LOADED_NOTIFICATION_DATA Loaded;
    LDR_DLL_LOADED_NOTIFICATION_DATA Unloaded;
} LDR_DLL_NOTIFICATION_DATA, *PLDR_DLL_NOTIFICATION_DATA;

typedef VOID(NTAPI* PLDR_DLL_NOTIFICATION_FUNCTION)(ULONG NotificationReason, PLDR_DLL_NOTIFICATION_DATA NotificationData, PVOID Context);
typedef NTSTATUS(NTAPI* LdrRegisterDllNotification_t)(ULONG Flags, PLDR_DLL_NOTIFICATION_FUNCTION NotificationFunction, PVOID Context, PVOID* Cookie);
typedef NTSTATUS(NTAPI* LdrUnregisterDllNotification_t)(PVOID Cookie);

static PVOID g_DllNotificationCookie = nullptr;
static LdrRegisterDllNotification_t OriginalLdrRegisterDllNotification = nullptr;
static LdrUnregisterDllNotification_t OriginalLdrUnregisterDllNotification = nullptr;

typedef HMODULE(WINAPI* LoadLibraryW_t)(LPCWSTR lpLibFileName);
typedef HMODULE(WINAPI* LoadLibraryA_t)(LPCSTR lpLibFileName);
typedef HMODULE(WINAPI* LoadLibraryExW_t)(LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags);
typedef HMODULE(WINAPI* LoadLibraryExA_t)(LPCSTR lpLibFileName, HANDLE hFile, DWORD dwFlags);
static LoadLibraryW_t OriginalLoadLibraryW = nullptr;
static LoadLibraryA_t OriginalLoadLibraryA = nullptr;
static LoadLibraryExW_t OriginalLoadLibraryExW = nullptr;
static LoadLibraryExA_t OriginalLoadLibraryExA = nullptr;

static bool IsRelevantModuleNameA(const char* name) {
    if (!name) return false;
    return strstr(name, "d3d11") || strstr(name, "D3D11") || strstr(name, "dxgi") || strstr(name, "DXGI");
}

static bool IsRelevantModuleNameW(LPCWSTR name) {
    if (!name) return false;
    char buf[256];
    WideCharToMultiByte(CP_ACP, 0, name, -1, buf, sizeof(buf), nullptr, nullptr);
    return IsRelevantModuleNameA(buf);
}

static void TryInitializeHooksForModuleName(const char* name) {
    if (!name || g_HooksInitialized.load()) return;
    if (IsRelevantModuleNameA(name)) {
        char buf[256];
        sprintf_s(buf, sizeof(buf), "[FrameForge] DETECTED module load: %s. Scheduling hook initialization...", name);
        LogToConsole(buf);
        g_HookActivationRequested.store(true);
    }
}

static VOID NTAPI LoaderNotificationCallback(ULONG NotificationReason, PLDR_DLL_NOTIFICATION_DATA NotificationData, PVOID Context) {
    if (NotificationReason != LdrDllNotificationReasonLoaded || !NotificationData) {
        return;
    }

    PCUNICODE_STRING moduleName = NotificationData->Loaded.BaseDllName;
    if (!moduleName || moduleName->Length == 0 || !moduleName->Buffer) {
        return;
    }

    int nameLength = moduleName->Length / sizeof(WCHAR);
    if (nameLength <= 0) {
        return;
    }

    char buf[256] = { 0 };
    if (nameLength >= sizeof(buf)) {
        nameLength = sizeof(buf) - 1;
    }

    WideCharToMultiByte(CP_ACP, 0, moduleName->Buffer, nameLength, buf, sizeof(buf), nullptr, nullptr);
    buf[nameLength] = '\0';
    TryInitializeHooksForModuleName(buf);
}

HMODULE WINAPI HookedLoadLibraryW(LPCWSTR lpLibFileName) {
    HMODULE result = OriginalLoadLibraryW(lpLibFileName);
    if (lpLibFileName) {
        char buf[256];
        WideCharToMultiByte(CP_ACP, 0, lpLibFileName, -1, buf, sizeof(buf), nullptr, nullptr);
        TryInitializeHooksForModuleName(buf);
    }
    return result;
}

HMODULE WINAPI HookedLoadLibraryA(LPCSTR lpLibFileName) {
    HMODULE result = OriginalLoadLibraryA(lpLibFileName);
    TryInitializeHooksForModuleName(lpLibFileName);
    return result;
}

HMODULE WINAPI HookedLoadLibraryExW(LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags) {
    HMODULE result = OriginalLoadLibraryExW(lpLibFileName, hFile, dwFlags);
    if (lpLibFileName) {
        char buf[256];
        WideCharToMultiByte(CP_ACP, 0, lpLibFileName, -1, buf, sizeof(buf), nullptr, nullptr);
        TryInitializeHooksForModuleName(buf);
    }
    return result;
}

HMODULE WINAPI HookedLoadLibraryExA(LPCSTR lpLibFileName, HANDLE hFile, DWORD dwFlags) {
    HMODULE result = OriginalLoadLibraryExA(lpLibFileName, hFile, dwFlags);
    TryInitializeHooksForModuleName(lpLibFileName);
    return result;
}

DWORD WINAPI MainThread(LPVOID lpParam) {
    __try {
        // Create console for debugging
        AllocConsole();
        FILE* f;
        freopen_s(&f, "CONOUT$", "w", stdout);

        LogToConsole("[FrameForge] Runtime Injected.");
        LogToConsole("[FrameForge] MainThread started.");

        __try {
            // Initialize MinHook early
            if (MH_Initialize() != MH_OK) {
                LogToConsole("[FrameForge] MH_Initialize failed!");
            } else {
                LogToConsole("[FrameForge] MinHook initialized.");
            }

            // Hook various LoadLibrary APIs to detect when d3d11/dxgi load
            HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
            if (hKernel32) {
                struct HookInfo { const char* name; void* func; void** original; } hooks[] = {
                    { "LoadLibraryW", (void*)&HookedLoadLibraryW, (void**)&OriginalLoadLibraryW },
                    { "LoadLibraryA", (void*)&HookedLoadLibraryA, (void**)&OriginalLoadLibraryA },
                    { "LoadLibraryExW", (void*)&HookedLoadLibraryExW, (void**)&OriginalLoadLibraryExW },
                    { "LoadLibraryExA", (void*)&HookedLoadLibraryExA, (void**)&OriginalLoadLibraryExA },
                };

                for (auto& hook : hooks) {
                FARPROC proc = GetProcAddress(hKernel32, hook.name);
                if (!proc) {
                    char buf[256];
                    sprintf_s(buf, sizeof(buf), "[FrameForge] %s not found.", hook.name);
                    LogToConsole(buf);
                    continue;
                }
                if (MH_CreateHook(proc, hook.func, hook.original) == MH_OK) {
                    if (MH_EnableHook(proc) == MH_OK) {
                        char buf[256];
                        sprintf_s(buf, sizeof(buf), "[FrameForge] Hooked %s successfully.", hook.name);
                        LogToConsole(buf);
                    } else {
                        char buf[256];
                        sprintf_s(buf, sizeof(buf), "[FrameForge] Failed to enable %s hook.", hook.name);
                        LogToConsole(buf);
                    }
                } else {
                    char buf[256];
                    sprintf_s(buf, sizeof(buf), "[FrameForge] Failed to create %s hook.", hook.name);
                    LogToConsole(buf);
                }
            }
        }

        // Register loader notification so we catch native DLL loads that bypass kernel32 LoadLibrary APIs.
        HMODULE hNtdll = GetModuleHandleA("ntdll.dll");
        if (hNtdll) {
            OriginalLdrRegisterDllNotification = reinterpret_cast<LdrRegisterDllNotification_t>(GetProcAddress(hNtdll, "LdrRegisterDllNotification"));
            OriginalLdrUnregisterDllNotification = reinterpret_cast<LdrUnregisterDllNotification_t>(GetProcAddress(hNtdll, "LdrUnregisterDllNotification"));
            if (OriginalLdrRegisterDllNotification) {
                NTSTATUS status = OriginalLdrRegisterDllNotification(0, LoaderNotificationCallback, nullptr, &g_DllNotificationCookie);
                if (status == 0 && g_DllNotificationCookie) {
                    LogToConsole("[FrameForge] Registered loader notification callback.");
                } else {
                    LogToConsole("[FrameForge] Failed to register loader notification callback.");
                }
            } else {
                LogToConsole("[FrameForge] LdrRegisterDllNotification unavailable.");
            }
        }

            // Also try immediate initialization if d3d11 is already loaded
            const int MAX_RETRIES = 120; // 2 minutes with 1-second intervals
            int retries = 0;

            while (retries < MAX_RETRIES && !g_HooksInitialized.load()) {
                HMODULE d3d11 = nullptr;
                bool shouldAttemptInit = g_HookActivationRequested.load();

                __try {
                    d3d11 = GetModuleHandleA("d3d11.dll");
                } __except (EXCEPTION_EXECUTE_HANDLER) {
                    LogToConsole("[FrameForge] SEH in GetModuleHandleA!");
                    retries++;
                    Sleep(1000);
                    continue;
                }

                if (d3d11) {
                    shouldAttemptInit = true;
                }

                if (shouldAttemptInit) {
                    LogToConsole("[FrameForge] Attempting hook initialization after relevant module load.");
                    __try {
                        if (FrameForge::Hooks::Initialize()) {
                            LogToConsole("[FrameForge] Hooks Initialized Successfully.");
                            g_HooksInitialized.store(true);
                            break;
                        } else {
                            char buf[256];
                            sprintf_s(buf, sizeof(buf), "[FrameForge] Failed to Initialize Hooks on attempt %d", retries + 1);
                            LogToConsole(buf);
                        }
                    } __except (EXCEPTION_EXECUTE_HANDLER) {
                        LogToConsole("[FrameForge] SEH in Initialize()!");
                    }
                } else {
                    retries++;
                    if (retries % 10 == 0) {
                        char buf[256];
                        sprintf_s(buf, sizeof(buf), "[FrameForge] Waiting for d3d11.dll... (attempt %d/%d)", retries, MAX_RETRIES);
                        LogToConsole(buf);
                    }
                }

                Sleep(1000);
            }

            if (!g_HooksInitialized) {
                char buf[256];
                sprintf_s(buf, sizeof(buf), "[FrameForge] Failed to initialize after %d attempts.", MAX_RETRIES);
                LogToConsole(buf);
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            LogToConsole("[FrameForge] SEH in main loop!");
        }

        LogToConsole("[FrameForge] MainThread completed.");
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        LogToFile("[FrameForge] FATAL: SEH in MainThread entry!");
    }

    return 0;
}

BOOL WINAPI DllMain(HINSTANCE hModule, DWORD dwReason, LPVOID lpReserved) {
    switch (dwReason) {
    case DLL_PROCESS_ATTACH:
        __try {
            OutputDebugStringA("[FrameForge] DllMain ATTACH");
            LogToFile("[FrameForge] DllMain ATTACH - DisableThreadLibraryCalls");
            DisableThreadLibraryCalls(hModule);
            
            HANDLE hThread = CreateThread(nullptr, 0, MainThread, hModule, 0, nullptr);
            if (!hThread) {
                LogToFile("[FrameForge] CreateThread failed!");
                OutputDebugStringA("[FrameForge] CreateThread failed!");
            } else {
                LogToFile("[FrameForge] Worker thread created successfully");
                OutputDebugStringA("[FrameForge] Worker thread created successfully");
                CloseHandle(hThread);
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            LogToFile("[FrameForge] SEH in DllMain ATTACH!");
            OutputDebugStringA("[FrameForge] SEH in DllMain ATTACH!");
        }
        break;
    case DLL_PROCESS_DETACH:
        __try {
            LogToFile("[FrameForge] DllMain DETACH - Shutdown");
            if (g_DllNotificationCookie && OriginalLdrUnregisterDllNotification) {
                OriginalLdrUnregisterDllNotification(g_DllNotificationCookie);
                g_DllNotificationCookie = nullptr;
            }
            FrameForge::Hooks::Shutdown();
            MH_Uninitialize();
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            LogToFile("[FrameForge] SEH in Shutdown!");
        }
        break;
    }
    return TRUE;
}
