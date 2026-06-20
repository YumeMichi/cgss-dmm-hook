#include "stdinclude.hpp"

#include <cstdarg>

namespace {
    HANDLE g_log_file = INVALID_HANDLE_VALUE;

    HANDLE get_log_file() {
        if (g_log_file != INVALID_HANDLE_VALUE) {
            return g_log_file;
        }

        char path[MAX_PATH] = {};
        DWORD len = GetModuleFileNameA(nullptr, path, MAX_PATH);
        if (len == 0 || len >= MAX_PATH) {
            return INVALID_HANDLE_VALUE;
        }

        char* last_sep = path;
        for (char* p = path; *p != '\0'; ++p) {
            if (*p == '\\' || *p == '/') {
                last_sep = p;
            }
        }
        *last_sep = '\0';
        lstrcatA(path, "\\cgss-dmm-hook.log");

        g_log_file = CreateFileA(
            path,
            FILE_APPEND_DATA,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            nullptr,
            OPEN_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            nullptr
        );
        return g_log_file;
    }
}

void hook_log(const char* message) {
    if (!message) {
        return;
    }

    OutputDebugStringA(message);
    OutputDebugStringA("\r\n");

    HANDLE log_file = get_log_file();
    if (log_file == INVALID_HANDLE_VALUE) {
        return;
    }

    DWORD ignored = 0;
    WriteFile(log_file, message, lstrlenA(message), &ignored, nullptr);
    WriteFile(log_file, "\r\n", 2, &ignored, nullptr);
}

void hook_logf(const char* format, ...) {
    if (!format) {
        return;
    }

    char buffer[1024] = {};
    va_list args;
    va_start(args, format);
    wvsprintfA(buffer, format, args);
    va_end(args);

    hook_log(buffer);
}
