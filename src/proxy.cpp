#include "stdinclude.hpp"

#include "config.hpp"
#include "hook.hpp"

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

namespace {
    constexpr DWORD kProxyInitDelayMs = 1000;
    constexpr const char* kHelperExeName = "cgss-borderless-helper.exe";
    constexpr const char* kGameExeName = "imascgstage.exe";
    HMODULE g_original = nullptr;
    volatile LONG g_helper_started = 0;

    bool is_game_process() {
        char path[MAX_PATH] = {};
        DWORD len = GetModuleFileNameA(nullptr, path, MAX_PATH);
        if (len == 0 || len >= MAX_PATH) {
            return false;
        }

        const char* base_name = path;
        for (const char* p = path; *p != '\0'; ++p) {
            if (*p == '\\' || *p == '/') {
                base_name = p + 1;
            }
        }
        return lstrcmpiA(base_name, kGameExeName) == 0;
    }

    std::string get_game_directory() {
        char path[MAX_PATH] = {};
        DWORD len = GetModuleFileNameA(nullptr, path, MAX_PATH);
        if (len == 0 || len >= MAX_PATH) {
            return {};
        }

        char* last_sep = path;
        for (char* p = path; *p != '\0'; ++p) {
            if (*p == '\\' || *p == '/') {
                last_sep = p;
            }
        }
        *last_sep = '\0';
        return path;
    }

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

    void start_borderless_helper() {
        if (!config::get().launch_borderless_helper) {
            hook_log("[cgss-dmm-hook] launch_borderless_helper=false, skipping helper launch");
            return;
        }
        if (InterlockedCompareExchange(&g_helper_started, 1, 0) != 0) {
            return;
        }

        const auto game_dir = get_game_directory();
        if (game_dir.empty()) {
            hook_log("[cgss-dmm-hook] failed to resolve game directory for helper launch");
            return;
        }

        std::string helper_path = game_dir;
        helper_path += "\\";
        helper_path += kHelperExeName;

        DWORD attributes = GetFileAttributesA(helper_path.c_str());
        if (attributes == INVALID_FILE_ATTRIBUTES || (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
            hook_log("[cgss-dmm-hook] borderless helper exe not found, skipping launch");
            return;
        }

        char command_line[512] = {};
        wsprintfA(command_line, "\"%s\" --pid %lu", helper_path.c_str(), GetCurrentProcessId());

        STARTUPINFOA startup_info = {};
        startup_info.cb = sizeof(startup_info);
        PROCESS_INFORMATION process_info = {};
        BOOL result = CreateProcessA(
            helper_path.c_str(),
            command_line,
            nullptr,
            nullptr,
            FALSE,
            CREATE_NO_WINDOW,
            nullptr,
            game_dir.c_str(),
            &startup_info,
            &process_info
        );
        if (!result) {
            hook_logf(
                "[cgss-dmm-hook] failed to launch borderless helper, error=%lu",
                static_cast<unsigned long>(GetLastError())
            );
            return;
        }

        CloseHandle(process_info.hThread);
        CloseHandle(process_info.hProcess);
        hook_log("[cgss-dmm-hook] launched borderless helper");
    }

    DWORD WINAPI init_thread(void*) {
        Sleep(kProxyInitDelayMs);
        if (!is_game_process()) {
            load_original_version_dll();
            hook_log("[cgss-dmm-hook] non-game process detected, skipping hook/helper startup");
            return 0;
        }
        config::load();
        load_original_version_dll();
        start_borderless_helper();
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
