#include "stdinclude.hpp"

#include "config.hpp"

namespace {
    using GetSchemeTypeFn = int (*)(const MethodInfo*);
    using GetStringFn = void* (*)(const MethodInfo*);
    using SetStringFn = void (*)(void*, const MethodInfo*);
    using SetSchemeModeFn = void (*)(int, const MethodInfo*);
    constexpr int kHttpSchemeType = 0;
    constexpr DWORD kHookPollIntervalMs = 500;
    constexpr DWORD kGameAssemblySettleDelayMs = 5000;
    constexpr int kHookPollMaxAttempts = 600;

    volatile LONG g_hook_installed = 0;
    GetSchemeTypeFn g_orig_get_scheme_type = nullptr;
    GetStringFn g_orig_get_application_server_url = nullptr;
    GetStringFn g_orig_get_tele_scope_server_url = nullptr;
    GetStringFn g_orig_get_concert_server_url = nullptr;
    GetStringFn g_orig_get_resource_server_url = nullptr;
    GetStringFn g_orig_custom_preference_get_application_server_url = nullptr;
    GetStringFn g_orig_custom_preference_get_tele_scope_server_url = nullptr;
    GetStringFn g_orig_custom_preference_get_concert_server_url = nullptr;
    GetStringFn g_orig_custom_preference_get_resource_server_url = nullptr;
    SetStringFn g_orig_custom_preference_set_application_server_url = nullptr;
    SetStringFn g_orig_custom_preference_set_tele_scope_server_url = nullptr;
    SetStringFn g_orig_custom_preference_set_concert_server_url = nullptr;
    SetStringFn g_orig_custom_preference_set_resource_server_url = nullptr;
    SetSchemeModeFn g_orig_custom_preference_set_scheme_mode = nullptr;
    volatile LONG g_logged_gameassembly = 0;
    volatile LONG g_exports_state = 0;
    ULONGLONG g_gameassembly_seen_tick = 0;
    volatile LONG g_scheme_hook_called = 0;
    volatile LONG g_application_hook_called = 0;
    volatile LONG g_tele_scope_hook_called = 0;
    volatile LONG g_concert_hook_called = 0;
    volatile LONG g_resource_hook_called = 0;
    volatile LONG g_cp_application_hook_called = 0;
    volatile LONG g_cp_tele_scope_hook_called = 0;
    volatile LONG g_cp_concert_hook_called = 0;
    volatile LONG g_cp_resource_hook_called = 0;
    volatile LONG g_cp_set_application_hook_called = 0;
    volatile LONG g_cp_set_tele_scope_hook_called = 0;
    volatile LONG g_cp_set_concert_hook_called = 0;
    volatile LONG g_cp_set_resource_hook_called = 0;
    volatile LONG g_cp_set_scheme_hook_called = 0;

    int get_scheme_type_hook(const MethodInfo*) {
        if (InterlockedCompareExchange(&g_scheme_hook_called, 1, 0) == 0) {
            hook_log("[cgss-dmm-hook] GetSchemeType hook invoked");
        }
        return kHttpSchemeType;
    }

    void* get_application_server_url_hook(const MethodInfo* method) {
        const auto& urls = config::get();
        if (InterlockedCompareExchange(&g_application_hook_called, 1, 0) == 0) {
            hook_log("[cgss-dmm-hook] GetApplicationServerUrl hook invoked");
        }
        if (!urls.api_url.empty()) {
            return il2cpp_symbols::new_string(urls.api_url.c_str());
        }
        return g_orig_get_application_server_url ? g_orig_get_application_server_url(method) : nullptr;
    }

    void* get_tele_scope_server_url_hook(const MethodInfo* method) {
        const auto& urls = config::get();
        if (InterlockedCompareExchange(&g_tele_scope_hook_called, 1, 0) == 0) {
            hook_log("[cgss-dmm-hook] GetTeleScopeServerUrl hook invoked");
        }
        if (!urls.api_url.empty()) {
            return il2cpp_symbols::new_string(urls.api_url.c_str());
        }
        return g_orig_get_tele_scope_server_url ? g_orig_get_tele_scope_server_url(method) : nullptr;
    }

    void* get_concert_server_url_hook(const MethodInfo* method) {
        const auto& urls = config::get();
        if (InterlockedCompareExchange(&g_concert_hook_called, 1, 0) == 0) {
            hook_log("[cgss-dmm-hook] GetConcertServerUrl hook invoked");
        }
        if (!urls.api_url.empty()) {
            return il2cpp_symbols::new_string(urls.api_url.c_str());
        }
        return g_orig_get_concert_server_url ? g_orig_get_concert_server_url(method) : nullptr;
    }

    void* get_resource_server_url_hook(const MethodInfo* method) {
        const auto& urls = config::get();
        if (InterlockedCompareExchange(&g_resource_hook_called, 1, 0) == 0) {
            hook_log("[cgss-dmm-hook] GetResourceServerUrl hook invoked");
        }
        if (!urls.asset_url.empty()) {
            return il2cpp_symbols::new_string(urls.asset_url.c_str());
        }
        return g_orig_get_resource_server_url ? g_orig_get_resource_server_url(method) : nullptr;
    }

    void* custom_preference_get_application_server_url_hook(const MethodInfo* method) {
        const auto& urls = config::get();
        if (InterlockedCompareExchange(&g_cp_application_hook_called, 1, 0) == 0) {
            hook_log("[cgss-dmm-hook] Cute.CustomPreference.GetApplicationServerURL hook invoked");
        }
        if (!urls.api_url.empty()) {
            return il2cpp_symbols::new_string(urls.api_url.c_str());
        }
        return g_orig_custom_preference_get_application_server_url
            ? g_orig_custom_preference_get_application_server_url(method)
            : nullptr;
    }

    void* custom_preference_get_tele_scope_server_url_hook(const MethodInfo* method) {
        const auto& urls = config::get();
        if (InterlockedCompareExchange(&g_cp_tele_scope_hook_called, 1, 0) == 0) {
            hook_log("[cgss-dmm-hook] Cute.CustomPreference.GetTeleScopeServerURL hook invoked");
        }
        if (!urls.api_url.empty()) {
            return il2cpp_symbols::new_string(urls.api_url.c_str());
        }
        return g_orig_custom_preference_get_tele_scope_server_url
            ? g_orig_custom_preference_get_tele_scope_server_url(method)
            : nullptr;
    }

    void* custom_preference_get_concert_server_url_hook(const MethodInfo* method) {
        const auto& urls = config::get();
        if (InterlockedCompareExchange(&g_cp_concert_hook_called, 1, 0) == 0) {
            hook_log("[cgss-dmm-hook] Cute.CustomPreference.GetConcertServerURL hook invoked");
        }
        if (!urls.api_url.empty()) {
            return il2cpp_symbols::new_string(urls.api_url.c_str());
        }
        return g_orig_custom_preference_get_concert_server_url
            ? g_orig_custom_preference_get_concert_server_url(method)
            : nullptr;
    }

    void* custom_preference_get_resource_server_url_hook(const MethodInfo* method) {
        const auto& urls = config::get();
        if (InterlockedCompareExchange(&g_cp_resource_hook_called, 1, 0) == 0) {
            hook_log("[cgss-dmm-hook] Cute.CustomPreference.GetResourceServerURL hook invoked");
        }
        if (!urls.asset_url.empty()) {
            return il2cpp_symbols::new_string(urls.asset_url.c_str());
        }
        return g_orig_custom_preference_get_resource_server_url
            ? g_orig_custom_preference_get_resource_server_url(method)
            : nullptr;
    }

    void custom_preference_set_application_server_url_hook(void* value, const MethodInfo* method) {
        const auto& urls = config::get();
        if (InterlockedCompareExchange(&g_cp_set_application_hook_called, 1, 0) == 0) {
            hook_log("[cgss-dmm-hook] Cute.CustomPreference.SetApplicationServerURL hook invoked");
        }
        if (g_orig_custom_preference_set_application_server_url) {
            if (!urls.api_url.empty()) {
                value = il2cpp_symbols::new_string(urls.api_url.c_str());
            }
            g_orig_custom_preference_set_application_server_url(value, method);
        }
    }

    void custom_preference_set_tele_scope_server_url_hook(void* value, const MethodInfo* method) {
        const auto& urls = config::get();
        if (InterlockedCompareExchange(&g_cp_set_tele_scope_hook_called, 1, 0) == 0) {
            hook_log("[cgss-dmm-hook] Cute.CustomPreference.SetTeleScopeServerURL hook invoked");
        }
        if (g_orig_custom_preference_set_tele_scope_server_url) {
            if (!urls.api_url.empty()) {
                value = il2cpp_symbols::new_string(urls.api_url.c_str());
            }
            g_orig_custom_preference_set_tele_scope_server_url(value, method);
        }
    }

    void custom_preference_set_concert_server_url_hook(void* value, const MethodInfo* method) {
        const auto& urls = config::get();
        if (InterlockedCompareExchange(&g_cp_set_concert_hook_called, 1, 0) == 0) {
            hook_log("[cgss-dmm-hook] Cute.CustomPreference.SetConcertServerURL hook invoked");
        }
        if (g_orig_custom_preference_set_concert_server_url) {
            if (!urls.api_url.empty()) {
                value = il2cpp_symbols::new_string(urls.api_url.c_str());
            }
            g_orig_custom_preference_set_concert_server_url(value, method);
        }
    }

    void custom_preference_set_resource_server_url_hook(void* value, const MethodInfo* method) {
        const auto& urls = config::get();
        if (InterlockedCompareExchange(&g_cp_set_resource_hook_called, 1, 0) == 0) {
            hook_log("[cgss-dmm-hook] Cute.CustomPreference.SetResourceServerURL hook invoked");
        }
        if (g_orig_custom_preference_set_resource_server_url) {
            if (!urls.asset_url.empty()) {
                value = il2cpp_symbols::new_string(urls.asset_url.c_str());
            }
            g_orig_custom_preference_set_resource_server_url(value, method);
        }
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
        const auto& urls = config::get();
        if (!urls.force_http) {
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
        auto custom_preference = il2cpp_symbols::get_class(
            "Assembly-CSharp.dll", "Cute", "CustomPreference"
        );
        if (!custom_preference) {
            hook_log("[cgss-dmm-hook] failed to resolve Cute.CustomPreference for URL overrides");
            return;
        }

        il2cpp_symbols::il2cpp_runtime_class_init(custom_preference);

        const auto& urls = config::get();
        if (urls.force_http) {
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
            auto api_value = il2cpp_symbols::new_string(urls.api_url.c_str());
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
            auto asset_value = il2cpp_symbols::new_string(urls.asset_url.c_str());
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

        patch_custom_preference_scheme();

        auto addr = il2cpp_symbols::get_method_pointer(
            "Assembly-CSharp.dll", "Stage", "NetworkUtil", "GetSchemeType", 0
        );
        if (!addr) {
            hook_log("[cgss-dmm-hook] failed to resolve Stage.NetworkUtil.GetSchemeType");
            return;
        }

        auto get_application_server_url_addr = il2cpp_symbols::get_method_pointer(
            "Assembly-CSharp.dll", "Stage", "NetworkUtil", "GetApplicationServerUrl", 0
        );
        auto get_tele_scope_server_url_addr = il2cpp_symbols::get_method_pointer(
            "Assembly-CSharp.dll", "Stage", "NetworkUtil", "GetTeleScopeServerUrl", 0
        );
        auto get_concert_server_url_addr = il2cpp_symbols::get_method_pointer(
            "Assembly-CSharp.dll", "Stage", "NetworkUtil", "GetConcertServerUrl", 0
        );
        auto get_resource_server_url_addr = il2cpp_symbols::get_method_pointer(
            "Assembly-CSharp.dll", "Stage", "NetworkUtil", "GetResourceServerUrl", 0
        );
        auto cp_get_application_server_url_addr = il2cpp_symbols::get_method_pointer(
            "Assembly-CSharp.dll", "Cute", "CustomPreference", "GetApplicationServerURL", 0
        );
        auto cp_get_tele_scope_server_url_addr = il2cpp_symbols::get_method_pointer(
            "Assembly-CSharp.dll", "Cute", "CustomPreference", "GetTeleScopeServerURL", 0
        );
        auto cp_get_concert_server_url_addr = il2cpp_symbols::get_method_pointer(
            "Assembly-CSharp.dll", "Cute", "CustomPreference", "GetConcertServerURL", 0
        );
        auto cp_get_resource_server_url_addr = il2cpp_symbols::get_method_pointer(
            "Assembly-CSharp.dll", "Cute", "CustomPreference", "GetResourceServerURL", 0
        );
        auto cp_set_application_server_url_addr = il2cpp_symbols::get_method_pointer(
            "Assembly-CSharp.dll", "Cute", "CustomPreference", "SetApplicationServerURL", 1
        );
        auto cp_set_tele_scope_server_url_addr = il2cpp_symbols::get_method_pointer(
            "Assembly-CSharp.dll", "Cute", "CustomPreference", "SetTeleScopeServerURL", 1
        );
        auto cp_set_concert_server_url_addr = il2cpp_symbols::get_method_pointer(
            "Assembly-CSharp.dll", "Cute", "CustomPreference", "SetConcertServerURL", 1
        );
        auto cp_set_resource_server_url_addr = il2cpp_symbols::get_method_pointer(
            "Assembly-CSharp.dll", "Cute", "CustomPreference", "SetResourceServerURL", 1
        );
        auto cp_set_scheme_mode_addr = il2cpp_symbols::get_method_pointer(
            "Assembly-CSharp.dll", "Cute", "CustomPreference", "SetScemeMode", 1
        );

        auto mh_status = MH_Initialize();
        if (mh_status != MH_OK && mh_status != MH_ERROR_ALREADY_INITIALIZED) {
            hook_log("[cgss-dmm-hook] MH_Initialize failed");
            return;
        }

        const auto& urls = config::get();
        if (urls.force_http) {
            auto create_status = MH_CreateHook(
                reinterpret_cast<void*>(addr),
                reinterpret_cast<void*>(&get_scheme_type_hook),
                reinterpret_cast<void**>(&g_orig_get_scheme_type)
            );
            if (create_status != MH_OK && create_status != MH_ERROR_ALREADY_CREATED) {
                hook_log("[cgss-dmm-hook] MH_CreateHook failed");
                return;
            }

            auto enable_status = MH_EnableHook(reinterpret_cast<void*>(addr));
            if (enable_status != MH_OK && enable_status != MH_ERROR_ENABLED) {
                hook_log("[cgss-dmm-hook] MH_EnableHook failed");
                return;
            }
        } else {
            hook_log("[cgss-dmm-hook] force_http disabled, keeping original scheme");
        }

        if (get_application_server_url_addr) {
            auto create_status = MH_CreateHook(
                reinterpret_cast<void*>(get_application_server_url_addr),
                reinterpret_cast<void*>(&get_application_server_url_hook),
                reinterpret_cast<void**>(&g_orig_get_application_server_url)
            );
            if (create_status == MH_OK || create_status == MH_ERROR_ALREADY_CREATED) {
                auto enable_status = MH_EnableHook(reinterpret_cast<void*>(get_application_server_url_addr));
                if (enable_status == MH_OK || enable_status == MH_ERROR_ENABLED) {
                    hook_log("[cgss-dmm-hook] hooked Stage.NetworkUtil.GetApplicationServerUrl");
                }
            }
        }

        if (get_resource_server_url_addr) {
            auto create_status = MH_CreateHook(
                reinterpret_cast<void*>(get_resource_server_url_addr),
                reinterpret_cast<void*>(&get_resource_server_url_hook),
                reinterpret_cast<void**>(&g_orig_get_resource_server_url)
            );
            if (create_status == MH_OK || create_status == MH_ERROR_ALREADY_CREATED) {
                auto enable_status = MH_EnableHook(reinterpret_cast<void*>(get_resource_server_url_addr));
                if (enable_status == MH_OK || enable_status == MH_ERROR_ENABLED) {
                    hook_log("[cgss-dmm-hook] hooked Stage.NetworkUtil.GetResourceServerUrl");
                }
            }
        }

        if (get_tele_scope_server_url_addr) {
            auto create_status = MH_CreateHook(
                reinterpret_cast<void*>(get_tele_scope_server_url_addr),
                reinterpret_cast<void*>(&get_tele_scope_server_url_hook),
                reinterpret_cast<void**>(&g_orig_get_tele_scope_server_url)
            );
            if (create_status == MH_OK || create_status == MH_ERROR_ALREADY_CREATED) {
                auto enable_status = MH_EnableHook(reinterpret_cast<void*>(get_tele_scope_server_url_addr));
                if (enable_status == MH_OK || enable_status == MH_ERROR_ENABLED) {
                    hook_log("[cgss-dmm-hook] hooked Stage.NetworkUtil.GetTeleScopeServerUrl");
                }
            }
        }

        if (get_concert_server_url_addr) {
            auto create_status = MH_CreateHook(
                reinterpret_cast<void*>(get_concert_server_url_addr),
                reinterpret_cast<void*>(&get_concert_server_url_hook),
                reinterpret_cast<void**>(&g_orig_get_concert_server_url)
            );
            if (create_status == MH_OK || create_status == MH_ERROR_ALREADY_CREATED) {
                auto enable_status = MH_EnableHook(reinterpret_cast<void*>(get_concert_server_url_addr));
                if (enable_status == MH_OK || enable_status == MH_ERROR_ENABLED) {
                    hook_log("[cgss-dmm-hook] hooked Stage.NetworkUtil.GetConcertServerUrl");
                }
            }
        }

        if (cp_get_application_server_url_addr) {
            auto create_status = MH_CreateHook(
                reinterpret_cast<void*>(cp_get_application_server_url_addr),
                reinterpret_cast<void*>(&custom_preference_get_application_server_url_hook),
                reinterpret_cast<void**>(&g_orig_custom_preference_get_application_server_url)
            );
            if (create_status == MH_OK || create_status == MH_ERROR_ALREADY_CREATED) {
                auto enable_status = MH_EnableHook(reinterpret_cast<void*>(cp_get_application_server_url_addr));
                if (enable_status == MH_OK || enable_status == MH_ERROR_ENABLED) {
                    hook_log("[cgss-dmm-hook] hooked Cute.CustomPreference.GetApplicationServerURL");
                }
            }
        }

        if (cp_get_tele_scope_server_url_addr) {
            auto create_status = MH_CreateHook(
                reinterpret_cast<void*>(cp_get_tele_scope_server_url_addr),
                reinterpret_cast<void*>(&custom_preference_get_tele_scope_server_url_hook),
                reinterpret_cast<void**>(&g_orig_custom_preference_get_tele_scope_server_url)
            );
            if (create_status == MH_OK || create_status == MH_ERROR_ALREADY_CREATED) {
                auto enable_status = MH_EnableHook(reinterpret_cast<void*>(cp_get_tele_scope_server_url_addr));
                if (enable_status == MH_OK || enable_status == MH_ERROR_ENABLED) {
                    hook_log("[cgss-dmm-hook] hooked Cute.CustomPreference.GetTeleScopeServerURL");
                }
            }
        }

        if (cp_get_concert_server_url_addr) {
            auto create_status = MH_CreateHook(
                reinterpret_cast<void*>(cp_get_concert_server_url_addr),
                reinterpret_cast<void*>(&custom_preference_get_concert_server_url_hook),
                reinterpret_cast<void**>(&g_orig_custom_preference_get_concert_server_url)
            );
            if (create_status == MH_OK || create_status == MH_ERROR_ALREADY_CREATED) {
                auto enable_status = MH_EnableHook(reinterpret_cast<void*>(cp_get_concert_server_url_addr));
                if (enable_status == MH_OK || enable_status == MH_ERROR_ENABLED) {
                    hook_log("[cgss-dmm-hook] hooked Cute.CustomPreference.GetConcertServerURL");
                }
            }
        }

        if (cp_get_resource_server_url_addr) {
            auto create_status = MH_CreateHook(
                reinterpret_cast<void*>(cp_get_resource_server_url_addr),
                reinterpret_cast<void*>(&custom_preference_get_resource_server_url_hook),
                reinterpret_cast<void**>(&g_orig_custom_preference_get_resource_server_url)
            );
            if (create_status == MH_OK || create_status == MH_ERROR_ALREADY_CREATED) {
                auto enable_status = MH_EnableHook(reinterpret_cast<void*>(cp_get_resource_server_url_addr));
                if (enable_status == MH_OK || enable_status == MH_ERROR_ENABLED) {
                    hook_log("[cgss-dmm-hook] hooked Cute.CustomPreference.GetResourceServerURL");
                }
            }
        }

        if (cp_set_application_server_url_addr) {
            auto create_status = MH_CreateHook(
                reinterpret_cast<void*>(cp_set_application_server_url_addr),
                reinterpret_cast<void*>(&custom_preference_set_application_server_url_hook),
                reinterpret_cast<void**>(&g_orig_custom_preference_set_application_server_url)
            );
            if (create_status == MH_OK || create_status == MH_ERROR_ALREADY_CREATED) {
                auto enable_status = MH_EnableHook(reinterpret_cast<void*>(cp_set_application_server_url_addr));
                if (enable_status == MH_OK || enable_status == MH_ERROR_ENABLED) {
                    hook_log("[cgss-dmm-hook] hooked Cute.CustomPreference.SetApplicationServerURL");
                }
            }
        }

        if (cp_set_tele_scope_server_url_addr) {
            auto create_status = MH_CreateHook(
                reinterpret_cast<void*>(cp_set_tele_scope_server_url_addr),
                reinterpret_cast<void*>(&custom_preference_set_tele_scope_server_url_hook),
                reinterpret_cast<void**>(&g_orig_custom_preference_set_tele_scope_server_url)
            );
            if (create_status == MH_OK || create_status == MH_ERROR_ALREADY_CREATED) {
                auto enable_status = MH_EnableHook(reinterpret_cast<void*>(cp_set_tele_scope_server_url_addr));
                if (enable_status == MH_OK || enable_status == MH_ERROR_ENABLED) {
                    hook_log("[cgss-dmm-hook] hooked Cute.CustomPreference.SetTeleScopeServerURL");
                }
            }
        }

        if (cp_set_concert_server_url_addr) {
            auto create_status = MH_CreateHook(
                reinterpret_cast<void*>(cp_set_concert_server_url_addr),
                reinterpret_cast<void*>(&custom_preference_set_concert_server_url_hook),
                reinterpret_cast<void**>(&g_orig_custom_preference_set_concert_server_url)
            );
            if (create_status == MH_OK || create_status == MH_ERROR_ALREADY_CREATED) {
                auto enable_status = MH_EnableHook(reinterpret_cast<void*>(cp_set_concert_server_url_addr));
                if (enable_status == MH_OK || enable_status == MH_ERROR_ENABLED) {
                    hook_log("[cgss-dmm-hook] hooked Cute.CustomPreference.SetConcertServerURL");
                }
            }
        }

        if (cp_set_resource_server_url_addr) {
            auto create_status = MH_CreateHook(
                reinterpret_cast<void*>(cp_set_resource_server_url_addr),
                reinterpret_cast<void*>(&custom_preference_set_resource_server_url_hook),
                reinterpret_cast<void**>(&g_orig_custom_preference_set_resource_server_url)
            );
            if (create_status == MH_OK || create_status == MH_ERROR_ALREADY_CREATED) {
                auto enable_status = MH_EnableHook(reinterpret_cast<void*>(cp_set_resource_server_url_addr));
                if (enable_status == MH_OK || enable_status == MH_ERROR_ENABLED) {
                    hook_log("[cgss-dmm-hook] hooked Cute.CustomPreference.SetResourceServerURL");
                }
            }
        }

        if (cp_set_scheme_mode_addr) {
            auto create_status = MH_CreateHook(
                reinterpret_cast<void*>(cp_set_scheme_mode_addr),
                reinterpret_cast<void*>(&custom_preference_set_scheme_mode_hook),
                reinterpret_cast<void**>(&g_orig_custom_preference_set_scheme_mode)
            );
            if (create_status == MH_OK || create_status == MH_ERROR_ALREADY_CREATED) {
                auto enable_status = MH_EnableHook(reinterpret_cast<void*>(cp_set_scheme_mode_addr));
                if (enable_status == MH_OK || enable_status == MH_ERROR_ENABLED) {
                    hook_log("[cgss-dmm-hook] hooked Cute.CustomPreference.SetScemeMode");
                }
            }
        }

        apply_custom_preference_url_overrides();

        InterlockedExchange(&g_hook_installed, 1);
        if (urls.force_http) {
            hook_log("[cgss-dmm-hook] hooked Stage.NetworkUtil.GetSchemeType");
        }
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
