#include "stdinclude.hpp"

#include "config.hpp"
#include "hook.hpp"

namespace {
    using GetSchemeTypeFn = int (*)(const MethodInfo*);
    using SetStringFn = void (*)(void*, const MethodInfo*);
    using SetSchemeModeFn = void (*)(int, const MethodInfo*);
    constexpr int kHttpSchemeType = 0;
    constexpr DWORD kHookPollIntervalMs = 500;
    constexpr DWORD kGameAssemblySettleDelayMs = 5000;
    constexpr int kHookPollMaxAttempts = 600;

    volatile LONG g_hook_installed = 0;
    GetSchemeTypeFn g_orig_get_scheme_type = nullptr;
    SetSchemeModeFn g_orig_custom_preference_set_scheme_mode = nullptr;
    volatile LONG g_logged_gameassembly = 0;
    volatile LONG g_exports_state = 0;
    ULONGLONG g_gameassembly_seen_tick = 0;
    volatile LONG g_scheme_hook_called = 0;
    volatile LONG g_cp_set_scheme_hook_called = 0;
    void* g_cached_api_url_string = nullptr;
    void* g_cached_asset_url_string = nullptr;

    void prepare_cached_override_strings() {
        const auto& urls = config::get();
        if (urls.api_url.empty() && urls.asset_url.empty()) {
            return;
        }
        if (!urls.api_url.empty() && !g_cached_api_url_string) {
            g_cached_api_url_string = il2cpp_symbols::new_string(urls.api_url.c_str());
            if (!g_cached_api_url_string) {
                hook_log("[cgss-dmm-hook] failed to cache api_url string");
            }
        }
        if (!urls.asset_url.empty() && !g_cached_asset_url_string) {
            g_cached_asset_url_string = il2cpp_symbols::new_string(urls.asset_url.c_str());
            if (!g_cached_asset_url_string) {
                hook_log("[cgss-dmm-hook] failed to cache asset_url string");
            }
        }
    }

    int get_scheme_type_hook(const MethodInfo*) {
        if (InterlockedCompareExchange(&g_scheme_hook_called, 1, 0) == 0) {
            hook_log("[cgss-dmm-hook] GetSchemeType hook invoked");
        }
        return kHttpSchemeType;
    }

    void custom_preference_set_scheme_mode_hook(int scheme_type, const MethodInfo* method) {
        if (InterlockedCompareExchange(&g_cp_set_scheme_hook_called, 1, 0) == 0) {
            hook_log("[cgss-dmm-hook] Cute.CustomPreference.SetScemeMode hook invoked");
        }
        if (g_orig_custom_preference_set_scheme_mode) {
            g_orig_custom_preference_set_scheme_mode(
                config::get().force_http ? kHttpSchemeType : scheme_type, method
            );
        }
    }

    void patch_custom_preference_scheme() {
        if (!config::get().force_http) {
            hook_log("[cgss-dmm-hook] force_http disabled, skipping CustomPreference scheme patch");
            return;
        }

        auto custom_preference = il2cpp_symbols::get_class(
            "Assembly-CSharp.dll", "Cute", "CustomPreference"
        );
        if (!custom_preference) {
            hook_log("[cgss-dmm-hook] failed to resolve Cute.CustomPreference");
            return;
        }

        if (il2cpp_symbols::set_static_int_field(custom_preference, "_schemeType", kHttpSchemeType)) {
            hook_log("[cgss-dmm-hook] patched Cute.CustomPreference._schemeType=0");
        } else {
            hook_log("[cgss-dmm-hook] failed to patch Cute.CustomPreference._schemeType");
        }
    }

    void apply_custom_preference_url_overrides() {
        const auto& urls = config::get();
        if (urls.api_url.empty() && urls.asset_url.empty()) {
            return;
        }

        auto custom_preference = il2cpp_symbols::get_class(
            "Assembly-CSharp.dll", "Cute", "CustomPreference"
        );
        if (!custom_preference) {
            hook_log("[cgss-dmm-hook] failed to resolve Cute.CustomPreference for URL overrides");
            return;
        }

        il2cpp_symbols::il2cpp_runtime_class_init(custom_preference);
        prepare_cached_override_strings();

        if (config::get().force_http) {
            auto set_scheme_mode_addr = il2cpp_symbols::get_method_pointer(
                "Assembly-CSharp.dll", "Cute", "CustomPreference", "SetScemeMode", 1
            );
            if (set_scheme_mode_addr) {
                auto set_scheme_mode = reinterpret_cast<SetSchemeModeFn>(set_scheme_mode_addr);
                set_scheme_mode(kHttpSchemeType, nullptr);
                hook_log("[cgss-dmm-hook] applied Cute.CustomPreference.SetScemeMode(0)");
            }
        }

        if (!urls.api_url.empty()) {
            auto set_application_addr = il2cpp_symbols::get_method_pointer(
                "Assembly-CSharp.dll", "Cute", "CustomPreference", "SetApplicationServerURL", 1
            );
            auto set_tele_scope_addr = il2cpp_symbols::get_method_pointer(
                "Assembly-CSharp.dll", "Cute", "CustomPreference", "SetTeleScopeServerURL", 1
            );
            auto set_concert_addr = il2cpp_symbols::get_method_pointer(
                "Assembly-CSharp.dll", "Cute", "CustomPreference", "SetConcertServerURL", 1
            );
            auto api_value = g_cached_api_url_string;
            if (set_application_addr && api_value) {
                reinterpret_cast<SetStringFn>(set_application_addr)(api_value, nullptr);
                hook_log("[cgss-dmm-hook] applied Cute.CustomPreference.SetApplicationServerURL");
            }
            if (set_tele_scope_addr && api_value) {
                reinterpret_cast<SetStringFn>(set_tele_scope_addr)(api_value, nullptr);
                hook_log("[cgss-dmm-hook] applied Cute.CustomPreference.SetTeleScopeServerURL");
            }
            if (set_concert_addr && api_value) {
                reinterpret_cast<SetStringFn>(set_concert_addr)(api_value, nullptr);
                hook_log("[cgss-dmm-hook] applied Cute.CustomPreference.SetConcertServerURL");
            }
        }

        if (!urls.asset_url.empty()) {
            auto set_resource_addr = il2cpp_symbols::get_method_pointer(
                "Assembly-CSharp.dll", "Cute", "CustomPreference", "SetResourceServerURL", 1
            );
            auto asset_value = g_cached_asset_url_string;
            if (set_resource_addr && asset_value) {
                reinterpret_cast<SetStringFn>(set_resource_addr)(asset_value, nullptr);
                hook_log("[cgss-dmm-hook] applied Cute.CustomPreference.SetResourceServerURL");
            }
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
            hook_log("[cgss-dmm-hook] GameAssembly.dll loaded");
            g_gameassembly_seen_tick = GetTickCount64();
        }
        if (g_gameassembly_seen_tick != 0 &&
            GetTickCount64() - g_gameassembly_seen_tick < kGameAssemblySettleDelayMs) {
            return;
        }

        il2cpp_symbols::init(game_assembly);
        if (!il2cpp_symbols::ready()) {
            if (InterlockedCompareExchange(&g_exports_state, 1, 0) == 0) {
                hook_log("[cgss-dmm-hook] il2cpp exports not ready");
            }
            return;
        }
        LONG prev_exports_state = InterlockedExchange(&g_exports_state, 2);
        if (prev_exports_state != 2) {
            hook_log("[cgss-dmm-hook] il2cpp exports ready");
        }

        if (config::get().force_http) {
            patch_custom_preference_scheme();
        }
        const auto& urls = config::get();
        if (!urls.api_url.empty() || !urls.asset_url.empty()) {
            prepare_cached_override_strings();
            apply_custom_preference_url_overrides();
        }

        if (!urls.force_http) {
            InterlockedExchange(&g_hook_installed, 1);
            hook_log("[cgss-dmm-hook] force_http disabled, keeping original scheme");
            return;
        }

        auto get_scheme_type_addr = il2cpp_symbols::get_method_pointer(
            "Assembly-CSharp.dll", "Stage", "NetworkUtil", "GetSchemeType", 0
        );
        if (!get_scheme_type_addr) {
            hook_log("[cgss-dmm-hook] failed to resolve Stage.NetworkUtil.GetSchemeType");
            return;
        }

        auto mh_status = MH_Initialize();
        if (mh_status != MH_OK && mh_status != MH_ERROR_ALREADY_INITIALIZED) {
            hook_log("[cgss-dmm-hook] MH_Initialize failed");
            return;
        }

        auto create_status = MH_CreateHook(
            reinterpret_cast<void*>(get_scheme_type_addr),
            reinterpret_cast<void*>(&get_scheme_type_hook),
            reinterpret_cast<void**>(&g_orig_get_scheme_type)
        );
        if (create_status != MH_OK && create_status != MH_ERROR_ALREADY_CREATED) {
            hook_log("[cgss-dmm-hook] MH_CreateHook failed");
            return;
        }

        auto enable_status = MH_EnableHook(reinterpret_cast<void*>(get_scheme_type_addr));
        if (enable_status != MH_OK && enable_status != MH_ERROR_ENABLED) {
            hook_log("[cgss-dmm-hook] MH_EnableHook failed");
            return;
        }

        InterlockedExchange(&g_hook_installed, 1);
        hook_log("[cgss-dmm-hook] hooked Stage.NetworkUtil.GetSchemeType");
    }

    DWORD WINAPI init_thread(void*) {
        for (int i = 0; i < kHookPollMaxAttempts; ++i) {
            try_install_hook();
            if (InterlockedCompareExchange(&g_hook_installed, 1, 1) != 0) {
                return 0;
            }
            Sleep(kHookPollIntervalMs);
        }
        hook_log("[cgss-dmm-hook] timed out waiting for hook target");
        return 0;
    }
}

void start_hook_thread() {
    config::load();
    CreateThread(nullptr, 0, init_thread, nullptr, 0, nullptr);
}
