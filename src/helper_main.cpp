#include <windows.h>
#include <shellapi.h>

#include <cstdarg>
#include <cstdio>
#include <fstream>
#include <string>

namespace {
    constexpr DWORD kWindowPollIntervalMs = 500;
    constexpr int kWindowPollMaxAttempts = 240;
    constexpr DWORD kHotkeyPollIntervalMs = 50;
    constexpr DWORD kMaintainIntervalMs = 500;
    constexpr DWORD kDelayedStyleApplyMs = 4;
    constexpr int kToggleKey = VK_F11;

    HWND g_main_window = nullptr;
    bool g_borderless_active = false;
    bool g_saved_state_valid = false;
    bool g_borderless_drift_logged = false;
    RECT g_saved_window_rect = {};
    LONG_PTR g_saved_style = 0;
    LONG_PTR g_saved_exstyle = 0;
    DWORD g_target_process_id = 0;

    std::wstring get_window_class_name(HWND hwnd) {
        wchar_t buffer[256] = {};
        int len = GetClassNameW(hwnd, buffer, static_cast<int>(std::size(buffer)));
        if (len <= 0) {
            return {};
        }
        return std::wstring(buffer, buffer + len);
    }

    void helper_logf(const char* format, ...) {
        char exe_path[MAX_PATH] = {};
        DWORD len = GetModuleFileNameA(nullptr, exe_path, MAX_PATH);
        if (len == 0 || len >= MAX_PATH) {
            return;
        }

        char* last_sep = exe_path;
        for (char* p = exe_path; *p != '\0'; ++p) {
            if (*p == '\\' || *p == '/') {
                last_sep = p;
            }
        }
        *last_sep = '\0';
        lstrcatA(exe_path, "\\cgss-borderless-helper.log");

        char message[1024] = {};
        va_list args;
        va_start(args, format);
        _vsnprintf_s(message, sizeof(message), _TRUNCATE, format, args);
        va_end(args);

        std::ofstream stream(exe_path, std::ios::app | std::ios::binary);
        if (!stream.is_open()) {
            return;
        }

        SYSTEMTIME st = {};
        GetLocalTime(&st);
        char line[1200] = {};
        _snprintf_s(
            line,
            sizeof(line),
            _TRUNCATE,
            "[%04u-%02u-%02u %02u:%02u:%02u] %s\r\n",
            static_cast<unsigned>(st.wYear),
            static_cast<unsigned>(st.wMonth),
            static_cast<unsigned>(st.wDay),
            static_cast<unsigned>(st.wHour),
            static_cast<unsigned>(st.wMinute),
            static_cast<unsigned>(st.wSecond),
            message
        );
        stream << line;
    }

    void set_dpi_awareness() {
        using SetProcessDpiAwarenessContextFn = BOOL(WINAPI*)(HANDLE);
        auto user32 = GetModuleHandleW(L"user32.dll");
        if (!user32) {
            user32 = LoadLibraryW(L"user32.dll");
        }

        if (user32) {
            auto set_context = reinterpret_cast<SetProcessDpiAwarenessContextFn>(
                GetProcAddress(user32, "SetProcessDpiAwarenessContext")
            );
            if (set_context && set_context(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)) {
                helper_logf("[cgss-borderless-helper] enabled per-monitor-v2 DPI awareness");
                return;
            }
        }

        SetProcessDPIAware();
        helper_logf("[cgss-borderless-helper] enabled legacy DPI awareness");
    }

    bool parse_arguments() {
        LPWSTR* argv = nullptr;
        int argc = 0;
        argv = CommandLineToArgvW(GetCommandLineW(), &argc);
        if (!argv) {
            return false;
        }

        bool ok = false;
        for (int i = 1; i + 1 < argc; ++i) {
            if (lstrcmpiW(argv[i], L"--pid") == 0) {
                g_target_process_id = wcstoul(argv[i + 1], nullptr, 10);
                ok = g_target_process_id != 0;
                break;
            }
        }

        LocalFree(argv);
        return ok;
    }

    struct WindowSearchContext {
        DWORD process_id = 0;
        HWND hwnd = nullptr;
    };

    BOOL CALLBACK enum_windows_proc(HWND hwnd, LPARAM lparam) {
        auto* context = reinterpret_cast<WindowSearchContext*>(lparam);
        if (!context) {
            return TRUE;
        }

        DWORD process_id = 0;
        GetWindowThreadProcessId(hwnd, &process_id);
        if (process_id != context->process_id) {
            return TRUE;
        }
        if (GetWindow(hwnd, GW_OWNER) != nullptr) {
            return TRUE;
        }

        LONG_PTR style = GetWindowLongPtrW(hwnd, GWL_STYLE);
        if ((style & WS_CHILD) != 0) {
            return TRUE;
        }

        context->hwnd = hwnd;
        return FALSE;
    }

    HWND find_main_window(DWORD process_id) {
        WindowSearchContext context;
        context.process_id = process_id;
        EnumWindows(&enum_windows_proc, reinterpret_cast<LPARAM>(&context));
        return context.hwnd;
    }

    bool get_monitor_rect(HWND hwnd, RECT& rect, MONITORINFOEXW* out_monitor_info = nullptr) {
        MONITORINFOEXW monitor_info = {};
        monitor_info.cbSize = sizeof(monitor_info);
        HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
        if (!monitor || !GetMonitorInfoW(monitor, &monitor_info)) {
            return false;
        }
        rect = monitor_info.rcMonitor;
        if (out_monitor_info) {
            *out_monitor_info = monitor_info;
        }
        return true;
    }

    void save_window_state(HWND hwnd) {
        if (g_saved_state_valid) {
            return;
        }

        GetWindowRect(hwnd, &g_saved_window_rect);
        g_saved_style = GetWindowLongPtrW(hwnd, GWL_STYLE);
        g_saved_exstyle = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
        g_saved_state_valid = true;
        helper_logf(
            "[cgss-borderless-helper] saved window rect=(%ld,%ld)-(%ld,%ld)",
            static_cast<long>(g_saved_window_rect.left),
            static_cast<long>(g_saved_window_rect.top),
            static_cast<long>(g_saved_window_rect.right),
            static_cast<long>(g_saved_window_rect.bottom)
        );
    }

    void apply_borderless_once(HWND hwnd) {
        RECT monitor_rect = {};
        MONITORINFOEXW monitor_info = {};
        if (!get_monitor_rect(hwnd, monitor_rect, &monitor_info)) {
            helper_logf("[cgss-borderless-helper] failed to query monitor rect");
            return;
        }

        save_window_state(hwnd);

        LONG_PTR style = GetWindowLongPtrW(hwnd, GWL_STYLE);
        LONG_PTR exstyle = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
        LONG_PTR new_style = style & ~(WS_CAPTION | WS_THICKFRAME | WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX);
        LONG_PTR new_exstyle = exstyle &
                               ~(WS_EX_DLGMODALFRAME | WS_EX_COMPOSITED | WS_EX_WINDOWEDGE |
                                 WS_EX_CLIENTEDGE | WS_EX_LAYERED | WS_EX_STATICEDGE |
                                 WS_EX_TOOLWINDOW | WS_EX_APPWINDOW);

        SetWindowPos(
            hwnd,
            nullptr,
            monitor_rect.left,
            monitor_rect.top,
            monitor_rect.right - monitor_rect.left,
            monitor_rect.bottom - monitor_rect.top,
            SWP_SHOWWINDOW | SWP_NOOWNERZORDER | SWP_NOSENDCHANGING
        );

        Sleep(kDelayedStyleApplyMs);
        SetWindowLongPtrW(hwnd, GWL_STYLE, new_style);
        SetWindowLongPtrW(hwnd, GWL_EXSTYLE, new_exstyle);
        SetWindowPos(
            hwnd,
            nullptr,
            monitor_rect.left,
            monitor_rect.top,
            monitor_rect.right - monitor_rect.left,
            monitor_rect.bottom - monitor_rect.top,
            SWP_FRAMECHANGED | SWP_SHOWWINDOW | SWP_NOOWNERZORDER | SWP_NOSENDCHANGING
        );

        RECT actual_rect = {};
        GetWindowRect(hwnd, &actual_rect);
        helper_logf(
            "[cgss-borderless-helper] applied borderless class=%ls monitor=%ls target=(%ld,%ld)-(%ld,%ld) actual=(%ld,%ld)-(%ld,%ld)",
            get_window_class_name(hwnd).c_str(),
            monitor_info.szDevice,
            static_cast<long>(monitor_rect.left),
            static_cast<long>(monitor_rect.top),
            static_cast<long>(monitor_rect.right),
            static_cast<long>(monitor_rect.bottom),
            static_cast<long>(actual_rect.left),
            static_cast<long>(actual_rect.top),
            static_cast<long>(actual_rect.right),
            static_cast<long>(actual_rect.bottom)
        );
    }

    bool needs_borderless_reapply(HWND hwnd) {
        RECT expected_rect = {};
        if (!get_monitor_rect(hwnd, expected_rect)) {
            return false;
        }

        RECT current_rect = {};
        if (!GetWindowRect(hwnd, &current_rect)) {
            return false;
        }

        LONG_PTR style = GetWindowLongPtrW(hwnd, GWL_STYLE);
        const bool has_border = (style & (WS_CAPTION | WS_THICKFRAME | WS_SYSMENU)) != 0;
        const bool rect_mismatch = current_rect.left != expected_rect.left ||
                                   current_rect.top != expected_rect.top ||
                                   current_rect.right != expected_rect.right ||
                                   current_rect.bottom != expected_rect.bottom;
        return has_border || rect_mismatch;
    }

    void restore_window(HWND hwnd) {
        if (!g_saved_state_valid) {
            return;
        }

        SetWindowLongPtrW(hwnd, GWL_STYLE, g_saved_style);
        SetWindowLongPtrW(hwnd, GWL_EXSTYLE, g_saved_exstyle);
        SetWindowPos(
            hwnd,
            nullptr,
            g_saved_window_rect.left,
            g_saved_window_rect.top,
            g_saved_window_rect.right - g_saved_window_rect.left,
            g_saved_window_rect.bottom - g_saved_window_rect.top,
            SWP_FRAMECHANGED | SWP_SHOWWINDOW | SWP_NOOWNERZORDER | SWP_NOSENDCHANGING
        );
        helper_logf("[cgss-borderless-helper] restored original window style and rect");
    }

    void toggle_borderless(HWND hwnd) {
        if (!hwnd) {
            return;
        }

        RECT monitor_rect = {};
        MONITORINFOEXW monitor_info = {};
        if (!get_monitor_rect(hwnd, monitor_rect, &monitor_info)) {
            helper_logf("[cgss-borderless-helper] failed to resolve monitor rect for toggle");
            return;
        }

        if (g_borderless_active) {
            g_borderless_active = false;
            g_borderless_drift_logged = false;
            restore_window(hwnd);
            return;
        }

        g_borderless_active = true;
        g_borderless_drift_logged = false;
        helper_logf(
            "[cgss-borderless-helper] borderless enabled monitor=%ls rect=(%ld,%ld)-(%ld,%ld)",
            monitor_info.szDevice,
            static_cast<long>(monitor_rect.left),
            static_cast<long>(monitor_rect.top),
            static_cast<long>(monitor_rect.right),
            static_cast<long>(monitor_rect.bottom)
        );
        apply_borderless_once(hwnd);
    }
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    set_dpi_awareness();

    if (!parse_arguments()) {
        helper_logf("[cgss-borderless-helper] missing or invalid --pid argument");
        return 1;
    }

    HANDLE target_process = OpenProcess(SYNCHRONIZE | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, g_target_process_id);
    if (!target_process) {
        helper_logf("[cgss-borderless-helper] failed to open target process");
        return 1;
    }

    for (int i = 0; i < kWindowPollMaxAttempts; ++i) {
        g_main_window = find_main_window(g_target_process_id);
        if (g_main_window) {
            helper_logf("[cgss-borderless-helper] found main window hwnd=0x%p", g_main_window);
            break;
        }
        if (WaitForSingleObject(target_process, 0) == WAIT_OBJECT_0) {
            CloseHandle(target_process);
            helper_logf("[cgss-borderless-helper] target process exited before main window appeared");
            return 0;
        }
        Sleep(kWindowPollIntervalMs);
    }

    if (!g_main_window) {
        CloseHandle(target_process);
        helper_logf("[cgss-borderless-helper] timed out waiting for main window");
        return 1;
    }

    SHORT previous_hotkey_state = 0;
    DWORD last_maintain_tick = GetTickCount();

    while (WaitForSingleObject(target_process, 0) != WAIT_OBJECT_0) {
        if (!IsWindow(g_main_window)) {
            helper_logf("[cgss-borderless-helper] target window no longer valid");
            break;
        }

        SHORT current_hotkey_state = GetAsyncKeyState(kToggleKey);
        bool is_pressed = (current_hotkey_state & 0x8000) != 0;
        bool was_pressed = (previous_hotkey_state & 0x8000) != 0;
        if (is_pressed && !was_pressed) {
            helper_logf("[cgss-borderless-helper] F11 pressed");
            toggle_borderless(g_main_window);
        }
        previous_hotkey_state = current_hotkey_state;

        DWORD now = GetTickCount();
        if (g_borderless_active && now - last_maintain_tick >= kMaintainIntervalMs) {
            if (needs_borderless_reapply(g_main_window)) {
                if (!g_borderless_drift_logged) {
                    helper_logf("[cgss-borderless-helper] detected borderless drift, reapplying");
                    g_borderless_drift_logged = true;
                }
                apply_borderless_once(g_main_window);
            } else {
                g_borderless_drift_logged = false;
            }
            last_maintain_tick = now;
        }

        Sleep(kHotkeyPollIntervalMs);
    }

    CloseHandle(target_process);
    helper_logf("[cgss-borderless-helper] target process exited, helper shutting down");
    return 0;
}
