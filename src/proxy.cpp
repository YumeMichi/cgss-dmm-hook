#include "stdinclude.hpp"

#include "config.hpp"
#include "hook.hpp"

namespace {
    void load_original_version_dll();
    void ensure_runtime_init_started();

    template <typename Result, typename Fn, typename... Args>
    Result forward_version_export(void*& original, Result failure_result, Args... args) {
        if (!original) {
            load_original_version_dll();
        }
        if (!original) {
            SetLastError(ERROR_PROC_NOT_FOUND);
            return failure_result;
        }

        Result result = reinterpret_cast<Fn>(original)(args...);
        ensure_runtime_init_started();
        return result;
    }
}

extern "C" {
    void* GetFileVersionInfoA_Original = nullptr;
    void* GetFileVersionInfoW_Original = nullptr;
    void* GetFileVersionInfoExA_Original = nullptr;
    void* GetFileVersionInfoExW_Original = nullptr;
    void* GetFileVersionInfoSizeA_Original = nullptr;
    void* GetFileVersionInfoSizeW_Original = nullptr;
    void* GetFileVersionInfoSizeExA_Original = nullptr;
    void* GetFileVersionInfoSizeExW_Original = nullptr;
    void* VerQueryValueA_Original = nullptr;
    void* VerQueryValueW_Original = nullptr;
    void* VerFindFileA_Original = nullptr;
    void* VerFindFileW_Original = nullptr;
    void* VerInstallFileA_Original = nullptr;
    void* VerInstallFileW_Original = nullptr;
    void* VerLanguageNameA_Original = nullptr;
    void* VerLanguageNameW_Original = nullptr;

    BOOL WINAPI GetFileVersionInfoA_EXPORT(LPCSTR lptstrFilename, DWORD dwHandle, DWORD dwLen, LPVOID lpData) {
        using Fn = BOOL(WINAPI*)(LPCSTR, DWORD, DWORD, LPVOID);
        return forward_version_export<BOOL, Fn>(
            GetFileVersionInfoA_Original, FALSE, lptstrFilename, dwHandle, dwLen, lpData
        );
    }

    BOOL WINAPI GetFileVersionInfoW_EXPORT(LPCWSTR lptstrFilename, DWORD dwHandle, DWORD dwLen, LPVOID lpData) {
        using Fn = BOOL(WINAPI*)(LPCWSTR, DWORD, DWORD, LPVOID);
        return forward_version_export<BOOL, Fn>(
            GetFileVersionInfoW_Original, FALSE, lptstrFilename, dwHandle, dwLen, lpData
        );
    }

    BOOL WINAPI GetFileVersionInfoExA_EXPORT(
        DWORD dwFlags, LPCSTR lpwstrFilename, DWORD dwHandle, DWORD dwLen, LPVOID lpData
    ) {
        using Fn = BOOL(WINAPI*)(DWORD, LPCSTR, DWORD, DWORD, LPVOID);
        return forward_version_export<BOOL, Fn>(
            GetFileVersionInfoExA_Original, FALSE, dwFlags, lpwstrFilename, dwHandle, dwLen, lpData
        );
    }

    BOOL WINAPI GetFileVersionInfoExW_EXPORT(
        DWORD dwFlags, LPCWSTR lpwstrFilename, DWORD dwHandle, DWORD dwLen, LPVOID lpData
    ) {
        using Fn = BOOL(WINAPI*)(DWORD, LPCWSTR, DWORD, DWORD, LPVOID);
        return forward_version_export<BOOL, Fn>(
            GetFileVersionInfoExW_Original, FALSE, dwFlags, lpwstrFilename, dwHandle, dwLen, lpData
        );
    }

    DWORD WINAPI GetFileVersionInfoSizeA_EXPORT(LPCSTR lptstrFilename, LPDWORD lpdwHandle) {
        using Fn = DWORD(WINAPI*)(LPCSTR, LPDWORD);
        return forward_version_export<DWORD, Fn>(
            GetFileVersionInfoSizeA_Original, 0, lptstrFilename, lpdwHandle
        );
    }

    DWORD WINAPI GetFileVersionInfoSizeW_EXPORT(LPCWSTR lptstrFilename, LPDWORD lpdwHandle) {
        using Fn = DWORD(WINAPI*)(LPCWSTR, LPDWORD);
        return forward_version_export<DWORD, Fn>(
            GetFileVersionInfoSizeW_Original, 0, lptstrFilename, lpdwHandle
        );
    }

    DWORD WINAPI GetFileVersionInfoSizeExA_EXPORT(DWORD dwFlags, LPCSTR lpwstrFilename, LPDWORD lpdwHandle) {
        using Fn = DWORD(WINAPI*)(DWORD, LPCSTR, LPDWORD);
        return forward_version_export<DWORD, Fn>(
            GetFileVersionInfoSizeExA_Original, 0, dwFlags, lpwstrFilename, lpdwHandle
        );
    }

    DWORD WINAPI GetFileVersionInfoSizeExW_EXPORT(DWORD dwFlags, LPCWSTR lpwstrFilename, LPDWORD lpdwHandle) {
        using Fn = DWORD(WINAPI*)(DWORD, LPCWSTR, LPDWORD);
        return forward_version_export<DWORD, Fn>(
            GetFileVersionInfoSizeExW_Original, 0, dwFlags, lpwstrFilename, lpdwHandle
        );
    }

    BOOL WINAPI VerQueryValueA_EXPORT(LPCVOID pBlock, LPCSTR lpSubBlock, LPVOID* lplpBuffer, PUINT puLen) {
        using Fn = BOOL(WINAPI*)(LPCVOID, LPCSTR, LPVOID*, PUINT);
        return forward_version_export<BOOL, Fn>(
            VerQueryValueA_Original, FALSE, pBlock, lpSubBlock, lplpBuffer, puLen
        );
    }

    BOOL WINAPI VerQueryValueW_EXPORT(LPCVOID pBlock, LPCWSTR lpSubBlock, LPVOID* lplpBuffer, PUINT puLen) {
        using Fn = BOOL(WINAPI*)(LPCVOID, LPCWSTR, LPVOID*, PUINT);
        return forward_version_export<BOOL, Fn>(
            VerQueryValueW_Original, FALSE, pBlock, lpSubBlock, lplpBuffer, puLen
        );
    }

    DWORD WINAPI VerFindFileA_EXPORT(
        DWORD uFlags, LPSTR szFileName, LPSTR szWinDir, LPSTR szAppDir,
        LPSTR szCurDir, PUINT lpuCurDirLen, LPSTR szDestDir, PUINT lpuDestDirLen
    ) {
        using Fn = DWORD(WINAPI*)(DWORD, LPSTR, LPSTR, LPSTR, LPSTR, PUINT, LPSTR, PUINT);
        return forward_version_export<DWORD, Fn>(
            VerFindFileA_Original,
            0,
            uFlags, szFileName, szWinDir, szAppDir, szCurDir, lpuCurDirLen, szDestDir, lpuDestDirLen
        );
    }

    DWORD WINAPI VerFindFileW_EXPORT(
        DWORD uFlags, LPWSTR szFileName, LPWSTR szWinDir, LPWSTR szAppDir,
        LPWSTR szCurDir, PUINT lpuCurDirLen, LPWSTR szDestDir, PUINT lpuDestDirLen
    ) {
        using Fn = DWORD(WINAPI*)(DWORD, LPWSTR, LPWSTR, LPWSTR, LPWSTR, PUINT, LPWSTR, PUINT);
        return forward_version_export<DWORD, Fn>(
            VerFindFileW_Original,
            0,
            uFlags, szFileName, szWinDir, szAppDir, szCurDir, lpuCurDirLen, szDestDir, lpuDestDirLen
        );
    }

    DWORD WINAPI VerInstallFileA_EXPORT(
        DWORD uFlags, LPSTR szSrcFileName, LPSTR szDestFileName, LPSTR szSrcDir,
        LPSTR szDestDir, LPSTR szCurDir, LPSTR szTmpFile, PUINT lpuTmpFileLen
    ) {
        using Fn = DWORD(WINAPI*)(DWORD, LPSTR, LPSTR, LPSTR, LPSTR, LPSTR, LPSTR, PUINT);
        return forward_version_export<DWORD, Fn>(
            VerInstallFileA_Original,
            0,
            uFlags, szSrcFileName, szDestFileName, szSrcDir, szDestDir, szCurDir, szTmpFile, lpuTmpFileLen
        );
    }

    DWORD WINAPI VerInstallFileW_EXPORT(
        DWORD uFlags, LPWSTR szSrcFileName, LPWSTR szDestFileName, LPWSTR szSrcDir,
        LPWSTR szDestDir, LPWSTR szCurDir, LPWSTR szTmpFile, PUINT lpuTmpFileLen
    ) {
        using Fn = DWORD(WINAPI*)(DWORD, LPWSTR, LPWSTR, LPWSTR, LPWSTR, LPWSTR, LPWSTR, PUINT);
        return forward_version_export<DWORD, Fn>(
            VerInstallFileW_Original,
            0,
            uFlags, szSrcFileName, szDestFileName, szSrcDir, szDestDir, szCurDir, szTmpFile, lpuTmpFileLen
        );
    }

    DWORD WINAPI VerLanguageNameA_EXPORT(DWORD wLang, LPSTR szLang, DWORD nSize) {
        using Fn = DWORD(WINAPI*)(DWORD, LPSTR, DWORD);
        return forward_version_export<DWORD, Fn>(
            VerLanguageNameA_Original, 0, wLang, szLang, nSize
        );
    }

    DWORD WINAPI VerLanguageNameW_EXPORT(DWORD wLang, LPWSTR szLang, DWORD nSize) {
        using Fn = DWORD(WINAPI*)(DWORD, LPWSTR, DWORD);
        return forward_version_export<DWORD, Fn>(
            VerLanguageNameW_Original, 0, wLang, szLang, nSize
        );
    }
}

namespace {
    constexpr DWORD kProxyInitDelayMs = 1000;
    constexpr const char* kHelperExeName = "cgss-borderless-helper.exe";
    constexpr const char* kGameExeName = "imascgstage.exe";
    HMODULE g_original = nullptr;
    volatile LONG g_helper_started = 0;
    volatile LONG g_runtime_init_started = 0;
    volatile LONG g_process_type = 0;

    bool is_game_process() {
        LONG process_type = InterlockedCompareExchange(&g_process_type, 0, 0);
        if (process_type != 0) {
            return process_type == 1;
        }

        char path[MAX_PATH] = {};
        DWORD len = GetModuleFileNameA(nullptr, path, MAX_PATH);
        if (len == 0 || len >= MAX_PATH) {
            InterlockedCompareExchange(&g_process_type, -1, 0);
            return false;
        }

        const char* base_name = path;
        for (const char* p = path; *p != '\0'; ++p) {
            if (*p == '\\' || *p == '/') {
                base_name = p + 1;
            }
        }
        bool is_game = lstrcmpiA(base_name, kGameExeName) == 0;
        InterlockedCompareExchange(&g_process_type, is_game ? 1 : -1, 0);
        return is_game;
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

        char path[MAX_PATH] = {};
        char system_dir[MAX_PATH] = {};
        UINT system_dir_len = GetSystemDirectoryA(system_dir, MAX_PATH);
        if (system_dir_len == 0 || system_dir_len >= MAX_PATH) {
            hook_log("[cgss-dmm-hook] failed to resolve system directory");
            return;
        }
        lstrcpynA(path, system_dir, MAX_PATH);
        lstrcatA(path, "\\version.dll");

        g_original = LoadLibraryA(path);
        if (!g_original) {
            hook_log("[cgss-dmm-hook] failed to load original version.dll");
            return;
        }

        GetFileVersionInfoA_Original = reinterpret_cast<void*>(GetProcAddress(g_original, "GetFileVersionInfoA"));
        GetFileVersionInfoW_Original = reinterpret_cast<void*>(GetProcAddress(g_original, "GetFileVersionInfoW"));
        GetFileVersionInfoExA_Original = reinterpret_cast<void*>(GetProcAddress(g_original, "GetFileVersionInfoExA"));
        GetFileVersionInfoExW_Original = reinterpret_cast<void*>(GetProcAddress(g_original, "GetFileVersionInfoExW"));
        GetFileVersionInfoSizeA_Original = reinterpret_cast<void*>(GetProcAddress(g_original, "GetFileVersionInfoSizeA"));
        GetFileVersionInfoSizeW_Original = reinterpret_cast<void*>(GetProcAddress(g_original, "GetFileVersionInfoSizeW"));
        GetFileVersionInfoSizeExA_Original = reinterpret_cast<void*>(GetProcAddress(g_original, "GetFileVersionInfoSizeExA"));
        GetFileVersionInfoSizeExW_Original = reinterpret_cast<void*>(GetProcAddress(g_original, "GetFileVersionInfoSizeExW"));
        VerQueryValueA_Original = reinterpret_cast<void*>(GetProcAddress(g_original, "VerQueryValueA"));
        VerQueryValueW_Original = reinterpret_cast<void*>(GetProcAddress(g_original, "VerQueryValueW"));
        VerFindFileA_Original = reinterpret_cast<void*>(GetProcAddress(g_original, "VerFindFileA"));
        VerFindFileW_Original = reinterpret_cast<void*>(GetProcAddress(g_original, "VerFindFileW"));
        VerInstallFileA_Original = reinterpret_cast<void*>(GetProcAddress(g_original, "VerInstallFileA"));
        VerInstallFileW_Original = reinterpret_cast<void*>(GetProcAddress(g_original, "VerInstallFileW"));
        VerLanguageNameA_Original = reinterpret_cast<void*>(GetProcAddress(g_original, "VerLanguageNameA"));
        VerLanguageNameW_Original = reinterpret_cast<void*>(GetProcAddress(g_original, "VerLanguageNameW"));

        if (!GetFileVersionInfoA_Original || !GetFileVersionInfoW_Original ||
            !GetFileVersionInfoExA_Original || !GetFileVersionInfoExW_Original ||
            !GetFileVersionInfoSizeA_Original || !GetFileVersionInfoSizeW_Original ||
            !GetFileVersionInfoSizeExA_Original || !GetFileVersionInfoSizeExW_Original ||
            !VerQueryValueA_Original || !VerQueryValueW_Original ||
            !VerFindFileA_Original || !VerFindFileW_Original ||
            !VerInstallFileA_Original || !VerInstallFileW_Original ||
            !VerLanguageNameA_Original || !VerLanguageNameW_Original) {
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
            hook_log("[cgss-dmm-hook] non-game process detected, skipping hook/helper startup");
            return 0;
        }
        if (!config::load()) {
            hook_log("[cgss-dmm-hook] config validation failed, terminating process");
            TerminateProcess(GetCurrentProcess(), 1);
            return 0;
        }
        start_borderless_helper();
        start_hook_thread();
        return 0;
    }

    void ensure_runtime_init_started() {
        if (!is_game_process()) {
            return;
        }
        if (InterlockedCompareExchange(&g_runtime_init_started, 1, 0) != 0) {
            return;
        }

        HANDLE thread = CreateThread(nullptr, 0, init_thread, nullptr, 0, nullptr);
        if (!thread) {
            hook_logf(
                "[cgss-dmm-hook] failed to create init thread, error=%lu",
                static_cast<unsigned long>(GetLastError())
            );
            return;
        }
        CloseHandle(thread);
    }
}

BOOL WINAPI DllMain(HINSTANCE module, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(module);
    }
    return TRUE;
}
