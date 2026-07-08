#include "stdinclude.hpp"

#include "config.hpp"
#include "hook.hpp"

namespace {
    using GetSchemeTypeFn = int (*)(const MethodInfo*);
    using GetStringFn = void* (*)(const MethodInfo*);
    using SetStringFn = void (*)(void*, const MethodInfo*);
    using SetSchemeModeFn = void (*)(int, const MethodInfo*);
    using ObscuredPrefsGetIntFn = int (*)(void*, int, const MethodInfo*);
    using ObscuredPrefsHasKeyFn = bool (*)(void*, const MethodInfo*);
    using ObscuredPrefsSetIntFn = void (*)(void*, int, const MethodInfo*);
    constexpr int kHttpSchemeType = 0;
    constexpr DWORD kHookPollIntervalMs = 500;
    constexpr DWORD kGameAssemblySettleDelayMs = 100;
    constexpr int kHookPollMaxAttempts = 600;
    constexpr LONG kPrefsHookLogLimit = 32;

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
    SetSchemeModeFn g_orig_custom_preference_set_scheme_mode = nullptr;
    SetStringFn g_orig_custom_preference_set_application_server_url = nullptr;
    SetStringFn g_orig_custom_preference_set_tele_scope_server_url = nullptr;
    SetStringFn g_orig_custom_preference_set_concert_server_url = nullptr;
    SetStringFn g_orig_custom_preference_set_resource_server_url = nullptr;
    ObscuredPrefsGetIntFn g_orig_obscured_prefs_get_int = nullptr;
    ObscuredPrefsHasKeyFn g_orig_obscured_prefs_has_key = nullptr;
    ObscuredPrefsSetIntFn g_orig_obscured_prefs_set_int = nullptr;
    volatile LONG g_logged_gameassembly = 0;
    volatile LONG g_exports_state = 0;
    ULONGLONG g_gameassembly_seen_tick = 0;
    volatile LONG g_scheme_hook_called = 0;
    volatile LONG g_cp_set_scheme_hook_called = 0;
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
    volatile LONG g_obscured_get_int_hook_calls = 0;
    volatile LONG g_obscured_has_key_hook_calls = 0;
    volatile LONG g_obscured_set_int_hook_calls = 0;

    struct ObscuredIntOverride {
        const char* key;
        int value;
    };

    constexpr ObscuredIntOverride kObscuredIntOverrides[] = {
        {"TUTORIAL_STEP", 1000},
        {"TUTORIAL_LOADED_FROM_SERVER_FLAG", 0},
        {"TUTORIAL_IS_DIRTY_LOCAL_DATA", 0},
        {"POLICY_PRE_ANNOUNCE_DATE", 1753452686},
        {"POLICY_ANNOUNCE_DATE", 1753452686},
        {"BN_CONTENT_RUN", 2},
        {"BN_CONTENT_AGREE_ANALYSIS", 1},
        {"BN_CONTENT_AGREE_ADVERTISEMENT", 1},
    };

    const char* il2cpp_string_chars(void* string_object) {
        if (!string_object) {
            return nullptr;
        }
        auto* raw = reinterpret_cast<const unsigned char*>(string_object);
        return reinterpret_cast<const char*>(raw + 16);
    }

    std::string il2cpp_string_to_utf8(void* string_object) {
        if (!string_object) {
            return {};
        }

        struct Il2CppStringLayout {
            void* klass;
            void* monitor;
            int32_t length;
            wchar_t chars[1];
        };

        auto* string_value = reinterpret_cast<const Il2CppStringLayout*>(string_object);
        if (string_value->length <= 0) {
            return {};
        }

        int size = WideCharToMultiByte(
            CP_UTF8,
            0,
            string_value->chars,
            string_value->length,
            nullptr,
            0,
            nullptr,
            nullptr
        );
        if (size <= 0) {
            return {};
        }

        std::string result(static_cast<size_t>(size), '\0');
        WideCharToMultiByte(
            CP_UTF8,
            0,
            string_value->chars,
            string_value->length,
            result.data(),
            size,
            nullptr,
            nullptr
        );
        return result;
    }

    bool lookup_obscured_int_override(const std::string& key, int& out_value) {
        if (key == "VIEWER_ID") {
            out_value = config::get().viewer_id;
            return true;
        }
        for (const auto& entry : kObscuredIntOverrides) {
            if (key == entry.key) {
                out_value = entry.value;
                return true;
            }
        }
        return false;
    }

    void* make_api_url_string() {
        const auto& urls = config::get();
        if (urls.api_url.empty()) {
            return nullptr;
        }
        return il2cpp_symbols::new_string(urls.api_url.c_str());
    }

    void* make_asset_url_string() {
        const auto& urls = config::get();
        if (urls.asset_url.empty()) {
            return nullptr;
        }
        return il2cpp_symbols::new_string(urls.asset_url.c_str());
    }

    int get_scheme_type_hook(const MethodInfo*) {
        if (InterlockedCompareExchange(&g_scheme_hook_called, 1, 0) == 0) {
            hook_log("[cgss-dmm-hook] GetSchemeType hook invoked");
        }
        return kHttpSchemeType;
    }

    void* get_application_server_url_hook(const MethodInfo* method) {
        if (InterlockedCompareExchange(&g_application_hook_called, 1, 0) == 0) {
            hook_log("[cgss-dmm-hook] Stage.NetworkUtil.GetApplicationServerUrl hook invoked");
        }
        if (auto override_value = make_api_url_string()) {
            return override_value;
        }
        return g_orig_get_application_server_url ? g_orig_get_application_server_url(method) : nullptr;
    }

    void* get_tele_scope_server_url_hook(const MethodInfo* method) {
        if (InterlockedCompareExchange(&g_tele_scope_hook_called, 1, 0) == 0) {
            hook_log("[cgss-dmm-hook] Stage.NetworkUtil.GetTeleScopeServerUrl hook invoked");
        }
        if (auto override_value = make_api_url_string()) {
            return override_value;
        }
        return g_orig_get_tele_scope_server_url ? g_orig_get_tele_scope_server_url(method) : nullptr;
    }

    void* get_concert_server_url_hook(const MethodInfo* method) {
        if (InterlockedCompareExchange(&g_concert_hook_called, 1, 0) == 0) {
            hook_log("[cgss-dmm-hook] Stage.NetworkUtil.GetConcertServerUrl hook invoked");
        }
        if (auto override_value = make_api_url_string()) {
            return override_value;
        }
        return g_orig_get_concert_server_url ? g_orig_get_concert_server_url(method) : nullptr;
    }

    void* get_resource_server_url_hook(const MethodInfo* method) {
        if (InterlockedCompareExchange(&g_resource_hook_called, 1, 0) == 0) {
            hook_log("[cgss-dmm-hook] Stage.NetworkUtil.GetResourceServerUrl hook invoked");
        }
        if (auto override_value = make_asset_url_string()) {
            return override_value;
        }
        return g_orig_get_resource_server_url ? g_orig_get_resource_server_url(method) : nullptr;
    }

    void* custom_preference_get_application_server_url_hook(const MethodInfo* method) {
        if (InterlockedCompareExchange(&g_cp_application_hook_called, 1, 0) == 0) {
            hook_log("[cgss-dmm-hook] Cute.CustomPreference.GetApplicationServerURL hook invoked");
        }
        if (auto override_value = make_api_url_string()) {
            return override_value;
        }
        return g_orig_custom_preference_get_application_server_url
            ? g_orig_custom_preference_get_application_server_url(method)
            : nullptr;
    }

    void* custom_preference_get_tele_scope_server_url_hook(const MethodInfo* method) {
        if (InterlockedCompareExchange(&g_cp_tele_scope_hook_called, 1, 0) == 0) {
            hook_log("[cgss-dmm-hook] Cute.CustomPreference.GetTeleScopeServerURL hook invoked");
        }
        if (auto override_value = make_api_url_string()) {
            return override_value;
        }
        return g_orig_custom_preference_get_tele_scope_server_url
            ? g_orig_custom_preference_get_tele_scope_server_url(method)
            : nullptr;
    }

    void* custom_preference_get_concert_server_url_hook(const MethodInfo* method) {
        if (InterlockedCompareExchange(&g_cp_concert_hook_called, 1, 0) == 0) {
            hook_log("[cgss-dmm-hook] Cute.CustomPreference.GetConcertServerURL hook invoked");
        }
        if (auto override_value = make_api_url_string()) {
            return override_value;
        }
        return g_orig_custom_preference_get_concert_server_url
            ? g_orig_custom_preference_get_concert_server_url(method)
            : nullptr;
    }

    void* custom_preference_get_resource_server_url_hook(const MethodInfo* method) {
        if (InterlockedCompareExchange(&g_cp_resource_hook_called, 1, 0) == 0) {
            hook_log("[cgss-dmm-hook] Cute.CustomPreference.GetResourceServerURL hook invoked");
        }
        if (auto override_value = make_asset_url_string()) {
            return override_value;
        }
        return g_orig_custom_preference_get_resource_server_url
            ? g_orig_custom_preference_get_resource_server_url(method)
            : nullptr;
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

    void custom_preference_set_application_server_url_hook(void* value, const MethodInfo* method) {
        if (InterlockedCompareExchange(&g_cp_set_application_hook_called, 1, 0) == 0) {
            hook_log("[cgss-dmm-hook] Cute.CustomPreference.SetApplicationServerURL hook invoked");
        }
        if (g_orig_custom_preference_set_application_server_url) {
            if (auto override_value = make_api_url_string()) {
                value = override_value;
            }
            g_orig_custom_preference_set_application_server_url(value, method);
        }
    }

    void custom_preference_set_tele_scope_server_url_hook(void* value, const MethodInfo* method) {
        if (InterlockedCompareExchange(&g_cp_set_tele_scope_hook_called, 1, 0) == 0) {
            hook_log("[cgss-dmm-hook] Cute.CustomPreference.SetTeleScopeServerURL hook invoked");
        }
        if (g_orig_custom_preference_set_tele_scope_server_url) {
            if (auto override_value = make_api_url_string()) {
                value = override_value;
            }
            g_orig_custom_preference_set_tele_scope_server_url(value, method);
        }
    }

    void custom_preference_set_concert_server_url_hook(void* value, const MethodInfo* method) {
        if (InterlockedCompareExchange(&g_cp_set_concert_hook_called, 1, 0) == 0) {
            hook_log("[cgss-dmm-hook] Cute.CustomPreference.SetConcertServerURL hook invoked");
        }
        if (g_orig_custom_preference_set_concert_server_url) {
            if (auto override_value = make_api_url_string()) {
                value = override_value;
            }
            g_orig_custom_preference_set_concert_server_url(value, method);
        }
    }

    void custom_preference_set_resource_server_url_hook(void* value, const MethodInfo* method) {
        if (InterlockedCompareExchange(&g_cp_set_resource_hook_called, 1, 0) == 0) {
            hook_log("[cgss-dmm-hook] Cute.CustomPreference.SetResourceServerURL hook invoked");
        }
        if (g_orig_custom_preference_set_resource_server_url) {
            if (auto override_value = make_asset_url_string()) {
                value = override_value;
            }
            g_orig_custom_preference_set_resource_server_url(value, method);
        }
    }

    int obscured_prefs_get_int_hook(void* key, int default_value, const MethodInfo* method) {
        auto key_string = il2cpp_string_to_utf8(key);
        int override_value = 0;
        bool matched = lookup_obscured_int_override(key_string, override_value);
        LONG count = InterlockedIncrement(&g_obscured_get_int_hook_calls);
        if (count <= kPrefsHookLogLimit) {
            hook_logf(
                "[cgss-dmm-hook] ObscuredPrefs.GetInt hook invoked key=%s matched=%s value=%d default=%d count=%ld",
                key_string.empty() ? "(null)" : key_string.c_str(),
                matched ? "true" : "false",
                matched ? override_value : default_value,
                default_value,
                count
            );
        }
        if (matched) {
            return override_value;
        }
        return g_orig_obscured_prefs_get_int ? g_orig_obscured_prefs_get_int(key, default_value, method)
                                             : default_value;
    }

    bool obscured_prefs_has_key_hook(void* key, const MethodInfo* method) {
        auto key_string = il2cpp_string_to_utf8(key);
        int override_value = 0;
        bool matched = lookup_obscured_int_override(key_string, override_value);
        LONG count = InterlockedIncrement(&g_obscured_has_key_hook_calls);
        if (count <= kPrefsHookLogLimit) {
            hook_logf(
                "[cgss-dmm-hook] ObscuredPrefs.HasKey hook invoked key=%s matched=%s count=%ld",
                key_string.empty() ? "(null)" : key_string.c_str(),
                matched ? "true" : "false",
                count
            );
        }
        if (matched) {
            return true;
        }
        return g_orig_obscured_prefs_has_key ? g_orig_obscured_prefs_has_key(key, method) : false;
    }

    void obscured_prefs_set_int_hook(void* key, int value, const MethodInfo* method) {
        auto key_string = il2cpp_string_to_utf8(key);
        int override_value = 0;
        bool matched = lookup_obscured_int_override(key_string, override_value);
        LONG count = InterlockedIncrement(&g_obscured_set_int_hook_calls);
        if (count <= kPrefsHookLogLimit) {
            hook_logf(
                "[cgss-dmm-hook] ObscuredPrefs.SetInt hook invoked key=%s matched=%s incoming=%d stored=%d count=%ld",
                key_string.empty() ? "(null)" : key_string.c_str(),
                matched ? "true" : "false",
                value,
                matched ? override_value : value,
                count
            );
        }
        if (g_orig_obscured_prefs_set_int) {
            g_orig_obscured_prefs_set_int(key, matched ? override_value : value, method);
        }
    }

    void patch_custom_preference_scheme(bool log_success = true) {
        if (!config::get().force_http) {
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
            if (log_success) {
                hook_log("[cgss-dmm-hook] patched Cute.CustomPreference._schemeType=0");
            }
        } else {
            hook_log("[cgss-dmm-hook] failed to patch Cute.CustomPreference._schemeType");
        }
    }

    void apply_custom_preference_url_overrides(bool log_success = true) {
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

        if (config::get().force_http) {
            auto set_scheme_mode_addr = il2cpp_symbols::get_method_pointer(
                "Assembly-CSharp.dll", "Cute", "CustomPreference", "SetScemeMode", 1
            );
            if (set_scheme_mode_addr) {
                auto set_scheme_mode = reinterpret_cast<SetSchemeModeFn>(set_scheme_mode_addr);
                set_scheme_mode(kHttpSchemeType, nullptr);
                if (log_success) {
                    hook_log("[cgss-dmm-hook] applied Cute.CustomPreference.SetScemeMode(0)");
                }
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
            auto api_value = make_api_url_string();
            if (set_application_addr && api_value) {
                reinterpret_cast<SetStringFn>(set_application_addr)(api_value, nullptr);
                if (log_success) {
                    hook_log("[cgss-dmm-hook] applied Cute.CustomPreference.SetApplicationServerURL");
                }
            }
            if (set_tele_scope_addr && api_value) {
                reinterpret_cast<SetStringFn>(set_tele_scope_addr)(api_value, nullptr);
                if (log_success) {
                    hook_log("[cgss-dmm-hook] applied Cute.CustomPreference.SetTeleScopeServerURL");
                }
            }
            if (set_concert_addr && api_value) {
                reinterpret_cast<SetStringFn>(set_concert_addr)(api_value, nullptr);
                if (log_success) {
                    hook_log("[cgss-dmm-hook] applied Cute.CustomPreference.SetConcertServerURL");
                }
            }
        }

        if (!urls.asset_url.empty()) {
            auto set_resource_addr = il2cpp_symbols::get_method_pointer(
                "Assembly-CSharp.dll", "Cute", "CustomPreference", "SetResourceServerURL", 1
            );
            auto asset_value = make_asset_url_string();
            if (set_resource_addr && asset_value) {
                reinterpret_cast<SetStringFn>(set_resource_addr)(asset_value, nullptr);
                if (log_success) {
                    hook_log("[cgss-dmm-hook] applied Cute.CustomPreference.SetResourceServerURL");
                }
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
            patch_custom_preference_scheme(true);
        }
        const auto& urls = config::get();
        if (!urls.api_url.empty() || !urls.asset_url.empty()) {
            apply_custom_preference_url_overrides(true);
        }

        uintptr_t get_scheme_type_addr = 0;
        uintptr_t cp_set_scheme_mode_addr = 0;
        if (urls.force_http) {
            get_scheme_type_addr = il2cpp_symbols::get_method_pointer(
                "Assembly-CSharp.dll", "Stage", "NetworkUtil", "GetSchemeType", 0
            );
            if (!get_scheme_type_addr) {
                hook_log("[cgss-dmm-hook] failed to resolve Stage.NetworkUtil.GetSchemeType");
                return;
            }
            cp_set_scheme_mode_addr = il2cpp_symbols::get_method_pointer(
                "Assembly-CSharp.dll", "Cute", "CustomPreference", "SetScemeMode", 1
            );
        } else {
            hook_log("[cgss-dmm-hook] force_http disabled, keeping original scheme");
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
        auto obscured_prefs_get_int_addr = il2cpp_symbols::get_method_pointer(
            "Assembly-CSharp.dll",
            "CodeStage.AntiCheat.ObscuredTypes",
            "ObscuredPrefs",
            "GetInt",
            2
        );
        auto obscured_prefs_has_key_addr = il2cpp_symbols::get_method_pointer(
            "Assembly-CSharp.dll",
            "CodeStage.AntiCheat.ObscuredTypes",
            "ObscuredPrefs",
            "HasKey",
            1
        );
        auto obscured_prefs_set_int_addr = il2cpp_symbols::get_method_pointer(
            "Assembly-CSharp.dll",
            "CodeStage.AntiCheat.ObscuredTypes",
            "ObscuredPrefs",
            "SetInt",
            2
        );

        auto mh_status = MH_Initialize();
        if (mh_status != MH_OK && mh_status != MH_ERROR_ALREADY_INITIALIZED) {
            hook_log("[cgss-dmm-hook] MH_Initialize failed");
            return;
        }

        if (urls.force_http) {
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
        }

        auto hook_method = [](uintptr_t address, void* detour, void** original, const char* name) {
            if (!address) {
                hook_logf("[cgss-dmm-hook] failed to resolve %s", name);
                return;
            }
            auto create_status = MH_CreateHook(reinterpret_cast<void*>(address), detour, original);
            if (create_status != MH_OK && create_status != MH_ERROR_ALREADY_CREATED) {
                hook_logf("[cgss-dmm-hook] MH_CreateHook failed for %s", name);
                return;
            }
            auto enable_status = MH_EnableHook(reinterpret_cast<void*>(address));
            if (enable_status == MH_OK || enable_status == MH_ERROR_ENABLED) {
                hook_logf("[cgss-dmm-hook] hooked %s", name);
            } else {
                hook_logf("[cgss-dmm-hook] MH_EnableHook failed for %s", name);
            }
        };

        if (urls.force_http) {
            hook_method(
                cp_set_scheme_mode_addr,
                reinterpret_cast<void*>(&custom_preference_set_scheme_mode_hook),
                reinterpret_cast<void**>(&g_orig_custom_preference_set_scheme_mode),
                "Cute.CustomPreference.SetScemeMode"
            );
        }
        if (!urls.api_url.empty()) {
            hook_method(
                get_application_server_url_addr,
                reinterpret_cast<void*>(&get_application_server_url_hook),
                reinterpret_cast<void**>(&g_orig_get_application_server_url),
                "Stage.NetworkUtil.GetApplicationServerUrl"
            );
            hook_method(
                get_tele_scope_server_url_addr,
                reinterpret_cast<void*>(&get_tele_scope_server_url_hook),
                reinterpret_cast<void**>(&g_orig_get_tele_scope_server_url),
                "Stage.NetworkUtil.GetTeleScopeServerUrl"
            );
            hook_method(
                get_concert_server_url_addr,
                reinterpret_cast<void*>(&get_concert_server_url_hook),
                reinterpret_cast<void**>(&g_orig_get_concert_server_url),
                "Stage.NetworkUtil.GetConcertServerUrl"
            );
            hook_method(
                cp_get_application_server_url_addr,
                reinterpret_cast<void*>(&custom_preference_get_application_server_url_hook),
                reinterpret_cast<void**>(&g_orig_custom_preference_get_application_server_url),
                "Cute.CustomPreference.GetApplicationServerURL"
            );
            hook_method(
                cp_get_tele_scope_server_url_addr,
                reinterpret_cast<void*>(&custom_preference_get_tele_scope_server_url_hook),
                reinterpret_cast<void**>(&g_orig_custom_preference_get_tele_scope_server_url),
                "Cute.CustomPreference.GetTeleScopeServerURL"
            );
            hook_method(
                cp_get_concert_server_url_addr,
                reinterpret_cast<void*>(&custom_preference_get_concert_server_url_hook),
                reinterpret_cast<void**>(&g_orig_custom_preference_get_concert_server_url),
                "Cute.CustomPreference.GetConcertServerURL"
            );
            hook_method(
                cp_set_application_server_url_addr,
                reinterpret_cast<void*>(&custom_preference_set_application_server_url_hook),
                reinterpret_cast<void**>(&g_orig_custom_preference_set_application_server_url),
                "Cute.CustomPreference.SetApplicationServerURL"
            );
            hook_method(
                cp_set_tele_scope_server_url_addr,
                reinterpret_cast<void*>(&custom_preference_set_tele_scope_server_url_hook),
                reinterpret_cast<void**>(&g_orig_custom_preference_set_tele_scope_server_url),
                "Cute.CustomPreference.SetTeleScopeServerURL"
            );
            hook_method(
                cp_set_concert_server_url_addr,
                reinterpret_cast<void*>(&custom_preference_set_concert_server_url_hook),
                reinterpret_cast<void**>(&g_orig_custom_preference_set_concert_server_url),
                "Cute.CustomPreference.SetConcertServerURL"
            );
        }
        if (!urls.asset_url.empty()) {
            hook_method(
                get_resource_server_url_addr,
                reinterpret_cast<void*>(&get_resource_server_url_hook),
                reinterpret_cast<void**>(&g_orig_get_resource_server_url),
                "Stage.NetworkUtil.GetResourceServerUrl"
            );
            hook_method(
                cp_get_resource_server_url_addr,
                reinterpret_cast<void*>(&custom_preference_get_resource_server_url_hook),
                reinterpret_cast<void**>(&g_orig_custom_preference_get_resource_server_url),
                "Cute.CustomPreference.GetResourceServerURL"
            );
            hook_method(
                cp_set_resource_server_url_addr,
                reinterpret_cast<void*>(&custom_preference_set_resource_server_url_hook),
                reinterpret_cast<void**>(&g_orig_custom_preference_set_resource_server_url),
                "Cute.CustomPreference.SetResourceServerURL"
            );
        }
        hook_method(
            obscured_prefs_get_int_addr,
            reinterpret_cast<void*>(&obscured_prefs_get_int_hook),
            reinterpret_cast<void**>(&g_orig_obscured_prefs_get_int),
            "CodeStage.AntiCheat.ObscuredTypes.ObscuredPrefs.GetInt"
        );
        hook_method(
            obscured_prefs_has_key_addr,
            reinterpret_cast<void*>(&obscured_prefs_has_key_hook),
            reinterpret_cast<void**>(&g_orig_obscured_prefs_has_key),
            "CodeStage.AntiCheat.ObscuredTypes.ObscuredPrefs.HasKey"
        );
        hook_method(
            obscured_prefs_set_int_addr,
            reinterpret_cast<void*>(&obscured_prefs_set_int_hook),
            reinterpret_cast<void**>(&g_orig_obscured_prefs_set_int),
            "CodeStage.AntiCheat.ObscuredTypes.ObscuredPrefs.SetInt"
        );

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
