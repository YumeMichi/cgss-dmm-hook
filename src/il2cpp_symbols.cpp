#include "stdinclude.hpp"

il2cpp_domain_get_t il2cpp_symbols::il2cpp_domain_get = nullptr;
il2cpp_domain_assembly_open_t il2cpp_symbols::il2cpp_domain_assembly_open = nullptr;
il2cpp_assembly_get_image_t il2cpp_symbols::il2cpp_assembly_get_image = nullptr;
il2cpp_class_from_name_t il2cpp_symbols::il2cpp_class_from_name = nullptr;
il2cpp_class_get_method_from_name_t il2cpp_symbols::il2cpp_class_get_method_from_name = nullptr;
il2cpp_class_get_methods_t il2cpp_symbols::il2cpp_class_get_methods = nullptr;
il2cpp_class_get_field_from_name_t il2cpp_symbols::il2cpp_class_get_field_from_name = nullptr;
il2cpp_method_get_name_t il2cpp_symbols::il2cpp_method_get_name = nullptr;
il2cpp_thread_attach_t il2cpp_symbols::il2cpp_thread_attach = nullptr;
il2cpp_thread_current_t il2cpp_symbols::il2cpp_thread_current = nullptr;
il2cpp_string_new_t il2cpp_symbols::il2cpp_string_new = nullptr;
il2cpp_runtime_class_init_t il2cpp_symbols::il2cpp_runtime_class_init = nullptr;
il2cpp_field_get_flags_t il2cpp_symbols::il2cpp_field_get_flags = nullptr;
il2cpp_field_static_set_value_t il2cpp_symbols::il2cpp_field_static_set_value = nullptr;
void* il2cpp_symbols::il2cpp_domain = nullptr;

namespace {
    template <typename T>
    void resolve(HMODULE module, const char* name, T& out) {
        out = reinterpret_cast<T>(GetProcAddress(module, name));
    }
}

void il2cpp_symbols::init(HMODULE game_module) {
    resolve(game_module, "il2cpp_domain_get", il2cpp_domain_get);
    resolve(game_module, "il2cpp_domain_assembly_open", il2cpp_domain_assembly_open);
    resolve(game_module, "il2cpp_assembly_get_image", il2cpp_assembly_get_image);
    resolve(game_module, "il2cpp_class_from_name", il2cpp_class_from_name);
    resolve(game_module, "il2cpp_class_get_method_from_name", il2cpp_class_get_method_from_name);
    resolve(game_module, "il2cpp_class_get_methods", il2cpp_class_get_methods);
    resolve(game_module, "il2cpp_class_get_field_from_name", il2cpp_class_get_field_from_name);
    resolve(game_module, "il2cpp_method_get_name", il2cpp_method_get_name);
    resolve(game_module, "il2cpp_thread_attach", il2cpp_thread_attach);
    resolve(game_module, "il2cpp_thread_current", il2cpp_thread_current);
    resolve(game_module, "il2cpp_string_new", il2cpp_string_new);
    resolve(game_module, "il2cpp_runtime_class_init", il2cpp_runtime_class_init);
    resolve(game_module, "il2cpp_field_get_flags", il2cpp_field_get_flags);
    resolve(game_module, "il2cpp_field_static_set_value", il2cpp_field_static_set_value);
}

bool il2cpp_symbols::ready() {
    return il2cpp_domain_get && il2cpp_domain_assembly_open && il2cpp_assembly_get_image &&
           il2cpp_class_from_name && il2cpp_class_get_method_from_name &&
           il2cpp_class_get_field_from_name && il2cpp_thread_attach &&
           il2cpp_string_new && il2cpp_runtime_class_init &&
           il2cpp_field_get_flags && il2cpp_field_static_set_value;
}

bool il2cpp_symbols::domain_ready() {
    if (!il2cpp_domain_get) {
        return false;
    }
    if (!il2cpp_domain) {
        il2cpp_domain = il2cpp_domain_get();
    }
    return il2cpp_domain != nullptr;
}

bool il2cpp_symbols::attach_thread() {
    if (!ready()) {
        return false;
    }
    if (!domain_ready()) {
        return false;
    }
    if (il2cpp_thread_current && il2cpp_thread_current()) {
        return true;
    }
    il2cpp_thread_attach(il2cpp_domain);
    return true;
}

void* il2cpp_symbols::new_string(const char* value) {
    if (!value || !attach_thread() || !il2cpp_string_new) {
        return nullptr;
    }
    return il2cpp_string_new(value);
}

void* il2cpp_symbols::get_class(const char* assembly_name, const char* namespaze, const char* klass_name) {
    if (!attach_thread()) {
        hook_log("[cgss-http-hook] il2cpp domain not ready");
        return nullptr;
    }
    if (!ready() || !il2cpp_domain) {
        return nullptr;
    }

    auto assembly = il2cpp_domain_assembly_open(il2cpp_domain, assembly_name);
    if (!assembly) {
        hook_log("[cgss-http-hook] assembly not found");
        return nullptr;
    }
    auto image = il2cpp_assembly_get_image(assembly);
    if (!image) {
        hook_log("[cgss-http-hook] image not found");
        return nullptr;
    }
    auto klass = il2cpp_class_from_name(image, namespaze, klass_name);
    if (!klass) {
        hook_log("[cgss-http-hook] class not found");
        return nullptr;
    }
    return klass;
}

bool il2cpp_symbols::set_static_int_field(void* klass, const char* field_name, int value) {
    if (!klass || !field_name || !attach_thread()) {
        return false;
    }

    il2cpp_runtime_class_init(klass);
    auto field = il2cpp_class_get_field_from_name(klass, field_name);
    if (!field) {
        hook_log("[cgss-http-hook] field not found");
        return false;
    }

    auto flags = il2cpp_field_get_flags(field);
    if ((flags & 0x10U) == 0) {
        hook_log("[cgss-http-hook] field is not static");
        return false;
    }

    il2cpp_field_static_set_value(field, &value);
    return true;
}

uintptr_t il2cpp_symbols::get_method_pointer(const char* assembly_name, const char* namespaze,
                                             const char* klass_name, const char* method_name, int args_count) {
    auto klass = get_class(assembly_name, namespaze, klass_name);
    if (!klass) {
        return 0;
    }
    auto method = il2cpp_class_get_method_from_name(klass, method_name, args_count);
    if (!method) {
        hook_log("[cgss-http-hook] method not found");
        return 0;
    }
    return method->methodPointer;
}
