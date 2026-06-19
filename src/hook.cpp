#include "stdinclude.hpp"

namespace {
    using GetSchemeTypeFn = int (*)();
    constexpr int kHttpSchemeType = 0;
    constexpr DWORD kHookPollIntervalMs = 500;
    constexpr DWORD kGameAssemblySettleDelayMs = 5000;
    constexpr int kHookPollMaxAttempts = 600;

    volatile LONG g_hook_installed = 0;
    GetSchemeTypeFn g_orig_get_scheme_type = nullptr;
    volatile LONG g_logged_gameassembly = 0;
    volatile LONG g_exports_state = 0;
    ULONGLONG g_gameassembly_seen_tick = 0;
    volatile LONG g_scheme_hook_called = 0;

    int get_scheme_type_hook() {
        if (InterlockedCompareExchange(&g_scheme_hook_called, 1, 0) == 0) {
            hook_log("[cgss-http-hook] GetSchemeType hook invoked");
        }
        return kHttpSchemeType;
    }

    void patch_custom_preference_scheme() {
        auto custom_preference = il2cpp_symbols::get_class(
            "Assembly-CSharp.dll", "Cute", "CustomPreference"
        );
        if (!custom_preference) {
            hook_log("[cgss-http-hook] failed to resolve Cute.CustomPreference");
            return;
        }

        if (il2cpp_symbols::set_static_int_field(custom_preference, "_schemeType", kHttpSchemeType)) {
            hook_log("[cgss-http-hook] patched Cute.CustomPreference._schemeType=0");
        } else {
            hook_log("[cgss-http-hook] failed to patch Cute.CustomPreference._schemeType");
        }
    }

    void try_install_hook() {
        if (InterlockedCompareExchange(&g_hook_installed, 1, 1) != 0) {
            return;
        }

        auto game_assembly = GetModuleHandleW(L"GameAssembly.dll");
        if (!game_assembly) {
            return;
        }
        if (InterlockedCompareExchange(&g_logged_gameassembly, 1, 0) == 0) {
            hook_log("[cgss-http-hook] GameAssembly.dll loaded");
            g_gameassembly_seen_tick = GetTickCount64();
        }
        if (g_gameassembly_seen_tick != 0 &&
            GetTickCount64() - g_gameassembly_seen_tick < kGameAssemblySettleDelayMs) {
            return;
        }

        il2cpp_symbols::init(game_assembly);
        if (!il2cpp_symbols::ready()) {
            if (InterlockedCompareExchange(&g_exports_state, 1, 0) == 0) {
                hook_log("[cgss-http-hook] il2cpp exports not ready");
            }
            return;
        }
        LONG prev_exports_state = InterlockedExchange(&g_exports_state, 2);
        if (prev_exports_state != 2) {
            hook_log("[cgss-http-hook] il2cpp exports ready");
        }

        patch_custom_preference_scheme();

        auto addr = il2cpp_symbols::get_method_pointer(
            "Assembly-CSharp.dll", "Stage", "NetworkUtil", "GetSchemeType", 0
        );
        if (!addr) {
            hook_log("[cgss-http-hook] failed to resolve Stage.NetworkUtil.GetSchemeType");
            return;
        }

        auto mh_status = MH_Initialize();
        if (mh_status != MH_OK && mh_status != MH_ERROR_ALREADY_INITIALIZED) {
            hook_log("[cgss-http-hook] MH_Initialize failed");
            return;
        }

        auto create_status = MH_CreateHook(
            reinterpret_cast<void*>(addr),
            reinterpret_cast<void*>(&get_scheme_type_hook),
            reinterpret_cast<void**>(&g_orig_get_scheme_type)
        );
        if (create_status != MH_OK && create_status != MH_ERROR_ALREADY_CREATED) {
            hook_log("[cgss-http-hook] MH_CreateHook failed");
            return;
        }

        auto enable_status = MH_EnableHook(reinterpret_cast<void*>(addr));
        if (enable_status != MH_OK && enable_status != MH_ERROR_ENABLED) {
            hook_log("[cgss-http-hook] MH_EnableHook failed");
            return;
        }

        InterlockedExchange(&g_hook_installed, 1);
        hook_log("[cgss-http-hook] hooked Stage.NetworkUtil.GetSchemeType");
    }

    DWORD WINAPI init_thread(void*) {
        for (int i = 0; i < kHookPollMaxAttempts; ++i) {
            try_install_hook();
            if (InterlockedCompareExchange(&g_hook_installed, 1, 1) != 0) {
                return 0;
            }
            Sleep(kHookPollIntervalMs);
        }
        hook_log("[cgss-http-hook] timed out waiting for hook target");
        return 0;
    }
}

void start_hook_thread() {
    CreateThread(nullptr, 0, init_thread, nullptr, 0, nullptr);
}
