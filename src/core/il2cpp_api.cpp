#include "il2cpp_api.h"

#include <windows.h>

#include <string_view>

namespace hackmatch::il2cpp {
namespace {
using domain_get_t = Domain* (*)();
using thread_attach_t = void* (*)(Domain*);
using domain_get_assemblies_t = Assembly** (*)(Domain*, std::size_t*);
using assembly_get_image_t = Image* (*)(Assembly*);
using image_get_name_t = const char* (*)(Image*);
using class_from_name_t = Class* (*)(Image*, const char*, const char*);
using method_from_name_t = MethodInfo* (*)(Class*, const char*, int);
using field_from_name_t = FieldInfo* (*)(Class*, const char*);
using field_get_offset_t = int (*)(FieldInfo*);
using field_get_value_t = void (*)(Object*, FieldInfo*, void*);
using field_set_value_t = void (*)(Object*, FieldInfo*, void*);
using field_static_get_value_t = void (*)(FieldInfo*, void*);
using field_static_set_value_t = void (*)(FieldInfo*, void*);
using class_get_type_t = const void* (*)(Class*);
using type_get_object_t = Object* (*)(const void*);
using runtime_invoke_t = Object* (*)(MethodInfo*, void*, void**, Object**);
using resolve_icall_t = void* (*)(const char*);

domain_get_t g_domain_get = nullptr;
thread_attach_t g_thread_attach = nullptr;
domain_get_assemblies_t g_domain_get_assemblies = nullptr;
assembly_get_image_t g_assembly_get_image = nullptr;
image_get_name_t g_image_get_name = nullptr;
class_from_name_t g_class_from_name = nullptr;
method_from_name_t g_method_from_name = nullptr;
field_from_name_t g_field_from_name = nullptr;
field_get_offset_t g_field_get_offset = nullptr;
field_get_value_t g_field_get_value = nullptr;
field_set_value_t g_field_set_value = nullptr;
field_static_get_value_t g_field_static_get_value = nullptr;
field_static_set_value_t g_field_static_set_value = nullptr;
class_get_type_t g_class_get_type = nullptr;
type_get_object_t g_type_get_object = nullptr;
runtime_invoke_t g_runtime_invoke = nullptr;
resolve_icall_t g_resolve_icall = nullptr;

bool g_ready = false;
const char* g_last_error = "not initialized";

template <class T>
T export_as(HMODULE module, const char* name)
{
    return reinterpret_cast<T>(GetProcAddress(module, name));
}

std::string_view trim_dll(std::string_view name)
{
    if (name.ends_with(".dll")) {
        name.remove_suffix(4);
    }
    return name;
}

bool same_assembly(std::string_view a, std::string_view b)
{
    return a == b || trim_dll(a) == trim_dll(b);
}
}

bool init()
{
    if (g_ready) {
        return true;
    }

    HMODULE game_assembly = GetModuleHandleA("GameAssembly.dll");
    if (!game_assembly) {
        g_last_error = "GameAssembly.dll is not loaded";
        return false;
    }

    g_domain_get = export_as<domain_get_t>(game_assembly, "il2cpp_domain_get");
    g_thread_attach = export_as<thread_attach_t>(game_assembly, "il2cpp_thread_attach");
    g_domain_get_assemblies = export_as<domain_get_assemblies_t>(game_assembly, "il2cpp_domain_get_assemblies");
    g_assembly_get_image = export_as<assembly_get_image_t>(game_assembly, "il2cpp_assembly_get_image");
    g_image_get_name = export_as<image_get_name_t>(game_assembly, "il2cpp_image_get_name");
    g_class_from_name = export_as<class_from_name_t>(game_assembly, "il2cpp_class_from_name");
    g_method_from_name = export_as<method_from_name_t>(game_assembly, "il2cpp_class_get_method_from_name");
    g_field_from_name = export_as<field_from_name_t>(game_assembly, "il2cpp_class_get_field_from_name");
    g_field_get_offset = export_as<field_get_offset_t>(game_assembly, "il2cpp_field_get_offset");
    g_field_get_value = export_as<field_get_value_t>(game_assembly, "il2cpp_field_get_value");
    g_field_set_value = export_as<field_set_value_t>(game_assembly, "il2cpp_field_set_value");
    g_field_static_get_value = export_as<field_static_get_value_t>(game_assembly, "il2cpp_field_static_get_value");
    g_field_static_set_value = export_as<field_static_set_value_t>(game_assembly, "il2cpp_field_static_set_value");
    g_class_get_type = export_as<class_get_type_t>(game_assembly, "il2cpp_class_get_type");
    g_type_get_object = export_as<type_get_object_t>(game_assembly, "il2cpp_type_get_object");
    g_runtime_invoke = export_as<runtime_invoke_t>(game_assembly, "il2cpp_runtime_invoke");
    g_resolve_icall = export_as<resolve_icall_t>(game_assembly, "il2cpp_resolve_icall");

    if (!g_domain_get || !g_thread_attach || !g_domain_get_assemblies || !g_assembly_get_image ||
        !g_image_get_name || !g_class_from_name || !g_method_from_name) {
        g_last_error = "missing core il2cpp exports";
        return false;
    }

    attach_thread();
    g_ready = true;
    g_last_error = "ok";
    return true;
}

bool ready()
{
    return g_ready;
}

const char* last_error()
{
    return g_last_error;
}

Domain* domain()
{
    return g_domain_get ? g_domain_get() : nullptr;
}

void attach_thread()
{
    if (g_thread_attach) {
        g_thread_attach(domain());
    }
}

Image* image(const char* assembly_name)
{
    if (!g_ready || !assembly_name) {
        return nullptr;
    }

    std::size_t count = 0;
    Assembly** assemblies = g_domain_get_assemblies(domain(), &count);
    for (std::size_t i = 0; assemblies && i < count; ++i) {
        Image* candidate = g_assembly_get_image(assemblies[i]);
        const char* name = candidate ? g_image_get_name(candidate) : nullptr;
        if (name && same_assembly(name, assembly_name)) {
            return candidate;
        }
    }

    return nullptr;
}

Class* klass(const char* assembly_name, const char* namespaze, const char* class_name)
{
    Image* img = image(assembly_name);
    return img ? g_class_from_name(img, namespaze ? namespaze : "", class_name) : nullptr;
}

MethodInfo* method(Class* cls, const char* method_name, int arg_count)
{
    if (!g_ready || !cls || !method_name) {
        return nullptr;
    }

    if (arg_count >= 0) {
        return g_method_from_name(cls, method_name, arg_count);
    }

    for (int i = 0; i < 16; ++i) {
        if (MethodInfo* found = g_method_from_name(cls, method_name, i)) {
            return found;
        }
    }

    return nullptr;
}

MethodInfo* method(const char* assembly_name, const char* namespaze, const char* class_name, const char* method_name, int arg_count)
{
    return method(klass(assembly_name, namespaze, class_name), method_name, arg_count);
}

FieldInfo* field(Class* cls, const char* field_name)
{
    return g_ready && g_field_from_name && cls && field_name ? g_field_from_name(cls, field_name) : nullptr;
}

int field_offset(FieldInfo* f)
{
    return g_ready && g_field_get_offset && f ? g_field_get_offset(f) : -1;
}

Object* type_object(Class* cls)
{
    const void* type = g_ready && g_class_get_type && cls ? g_class_get_type(cls) : nullptr;
    return type && g_type_get_object ? g_type_get_object(type) : nullptr;
}

Array* objects_of_type(Class* cls)
{
    MethodInfo* find_objects = method("UnityEngine.CoreModule", "UnityEngine", "Object", "FindObjectsOfType", 1);
    Object* type = type_object(cls);
    if (!find_objects || !type) {
        return nullptr;
    }

    void* args[] = {type};
    Object* exception = nullptr;
    Object* result = runtime_invoke(find_objects, nullptr, args, &exception);
    return exception ? nullptr : reinterpret_cast<Array*>(result);
}

bool read_field(Object* object, FieldInfo* f, void* value)
{
    if (!g_ready || !g_field_get_value || !object || !f || !value) {
        return false;
    }

    g_field_get_value(object, f, value);
    return true;
}

bool write_field(Object* object, FieldInfo* f, const void* value)
{
    if (!g_ready || !g_field_set_value || !object || !f || !value) {
        return false;
    }

    g_field_set_value(object, f, const_cast<void*>(value));
    return true;
}

bool read_static_field(FieldInfo* f, void* value)
{
    if (!g_ready || !g_field_static_get_value || !f || !value) {
        return false;
    }

    g_field_static_get_value(f, value);
    return true;
}

bool write_static_field(FieldInfo* f, const void* value)
{
    if (!g_ready || !f || !value) {
        return false;
    }

    if (g_field_static_set_value) {
        g_field_static_set_value(f, const_cast<void*>(value));
        return true;
    }

    return false;
}

Object* runtime_invoke(MethodInfo* m, void* object, void** args, Object** exception)
{
    return g_ready && g_runtime_invoke && m ? g_runtime_invoke(m, object, args, exception) : nullptr;
}

void* icall(const char* name)
{
    return g_ready && g_resolve_icall && name ? g_resolve_icall(name) : nullptr;
}
}
