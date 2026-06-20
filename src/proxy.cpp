#include "stdinclude.hpp"

extern "C" {
    void* GetFileVersionInfoA_Original = nullptr;
    void* GetFileVersionInfoSizeA_Original = nullptr;
    void* VerQueryValueA_Original = nullptr;

    BOOL WINAPI GetFileVersionInfoA_EXPORT(LPCSTR lptstrFilename, DWORD dwHandle, DWORD dwLen, LPVOID lpData) {
        using Fn = BOOL(WINAPI*)(LPCSTR, DWORD, DWORD, LPVOID);
        if (!GetFileVersionInfoA_Original) {
            SetLastError(ERROR_PROC_NOT_FOUND);
            return FALSE;
        }
        return reinterpret_cast<Fn>(GetFileVersionInfoA_Original)(lptstrFilename, dwHandle, dwLen, lpData);
    }

    DWORD WINAPI GetFileVersionInfoSizeA_EXPORT(LPCSTR lptstrFilename, LPDWORD lpdwHandle) {
        using Fn = DWORD(WINAPI*)(LPCSTR, LPDWORD);
        if (!GetFileVersionInfoSizeA_Original) {
            SetLastError(ERROR_PROC_NOT_FOUND);
            return 0;
        }
        return reinterpret_cast<Fn>(GetFileVersionInfoSizeA_Original)(lptstrFilename, lpdwHandle);
    }

    BOOL WINAPI VerQueryValueA_EXPORT(LPCVOID pBlock, LPCSTR lpSubBlock, LPVOID* lplpBuffer, PUINT puLen) {
        using Fn = BOOL(WINAPI*)(LPCVOID, LPCSTR, LPVOID*, PUINT);
        if (!VerQueryValueA_Original) {
            SetLastError(ERROR_PROC_NOT_FOUND);
            return FALSE;
        }
        return reinterpret_cast<Fn>(VerQueryValueA_Original)(pBlock, lpSubBlock, lplpBuffer, puLen);
    }
}

void start_hook_thread();

namespace {
    constexpr DWORD kProxyInitDelayMs = 1000;
    HMODULE g_original = nullptr;

    void load_original_version_dll() {
        if (g_original) {
            return;
        }

        char system_dir[MAX_PATH] = {};
        GetSystemDirectoryA(system_dir, MAX_PATH);
        char path[MAX_PATH] = {};
        lstrcpynA(path, system_dir, MAX_PATH);
        lstrcatA(path, "\\version.dll");

        g_original = LoadLibraryA(path);
        if (!g_original) {
            hook_log("[cgss-dmm-hook] failed to load original version.dll");
            return;
        }

        GetFileVersionInfoA_Original = reinterpret_cast<void*>(GetProcAddress(g_original, "GetFileVersionInfoA"));
        GetFileVersionInfoSizeA_Original = reinterpret_cast<void*>(GetProcAddress(g_original, "GetFileVersionInfoSizeA"));
        VerQueryValueA_Original = reinterpret_cast<void*>(GetProcAddress(g_original, "VerQueryValueA"));

        if (!GetFileVersionInfoA_Original || !GetFileVersionInfoSizeA_Original || !VerQueryValueA_Original) {
            hook_log("[cgss-dmm-hook] failed to resolve version exports");
            return;
        }

        hook_log("[cgss-dmm-hook] version.dll initialized");
    }

    DWORD WINAPI init_thread(void*) {
        Sleep(kProxyInitDelayMs);
        load_original_version_dll();
        start_hook_thread();
        return 0;
    }
}

BOOL WINAPI DllMain(HINSTANCE module, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(module);
        CreateThread(nullptr, 0, init_thread, nullptr, 0, nullptr);
    }
    return TRUE;
}
