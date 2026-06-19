#pragma once

#include <stdint.h>

struct MethodInfo;
struct FieldInfo;

typedef void* (*il2cpp_domain_get_t)();
typedef void* (*il2cpp_domain_assembly_open_t)(void* domain, const char* name);
typedef void* (*il2cpp_assembly_get_image_t)(void* assembly);
typedef void* (*il2cpp_class_from_name_t)(void* image, const char* namespaze, const char* name);
typedef MethodInfo* (*il2cpp_class_get_method_from_name_t)(void* klass, const char* name, int argsCount);
typedef MethodInfo* (*il2cpp_class_get_methods_t)(void* klass, void** iter);
typedef FieldInfo* (*il2cpp_class_get_field_from_name_t)(void* klass, const char* name);
typedef const char* (*il2cpp_method_get_name_t)(MethodInfo* method);
typedef void (*il2cpp_thread_attach_t)(void* domain);
typedef void* (*il2cpp_thread_current_t)();
typedef void* (*il2cpp_string_new_t)(const char* str);
typedef void* (*il2cpp_object_new_t)(void* klass);
typedef void (*il2cpp_runtime_class_init_t)(void* klass);
typedef uint32_t (*il2cpp_field_get_flags_t)(FieldInfo* field);
typedef void (*il2cpp_field_static_set_value_t)(FieldInfo* field, void* value);

struct MethodInfo
{
    uintptr_t methodPointer;
    uintptr_t invoker_method;
    const char* name;
    uintptr_t klass;
    const void* return_type;
    const void* parameters;
    uintptr_t methodDefinition;
    uintptr_t genericContainer;
    uint32_t token;
    uint16_t flags;
    uint16_t iflags;
    uint16_t slot;
    uint8_t parameters_count;
    uint8_t is_generic : 1;
    uint8_t is_inflated : 1;
    uint8_t wrapper_type : 1;
    uint8_t is_marshaled_from_native : 1;
};

namespace il2cpp_symbols {
    extern il2cpp_domain_get_t il2cpp_domain_get;
    extern il2cpp_domain_assembly_open_t il2cpp_domain_assembly_open;
    extern il2cpp_assembly_get_image_t il2cpp_assembly_get_image;
    extern il2cpp_class_from_name_t il2cpp_class_from_name;
    extern il2cpp_class_get_method_from_name_t il2cpp_class_get_method_from_name;
    extern il2cpp_class_get_methods_t il2cpp_class_get_methods;
    extern il2cpp_class_get_field_from_name_t il2cpp_class_get_field_from_name;
    extern il2cpp_method_get_name_t il2cpp_method_get_name;
    extern il2cpp_thread_attach_t il2cpp_thread_attach;
    extern il2cpp_thread_current_t il2cpp_thread_current;
    extern il2cpp_string_new_t il2cpp_string_new;
    extern il2cpp_object_new_t il2cpp_object_new;
    extern il2cpp_runtime_class_init_t il2cpp_runtime_class_init;
    extern il2cpp_field_get_flags_t il2cpp_field_get_flags;
    extern il2cpp_field_static_set_value_t il2cpp_field_static_set_value;
    extern void* il2cpp_domain;

    void init(HMODULE game_module);
    bool ready();
    bool domain_ready();
    bool attach_thread();
    void* new_string(const char* value);
    void* get_class(const char* assembly_name, const char* namespaze, const char* klass_name);
    bool set_static_int_field(void* klass, const char* field_name, int value);
    bool set_static_string_field(void* klass, const char* field_name, const char* value);
    uintptr_t get_method_pointer(const char* assembly_name, const char* namespaze,
                                 const char* klass_name, const char* method_name, int args_count);
}
