#pragma once

#include <cstddef>
#include <cstdint>

namespace hackmatch::il2cpp {
struct Domain;
struct Assembly;
struct Image;
struct Class;
struct FieldInfo;
}

using Il2CppMethodPointer = void (*)();

struct MethodInfo {
    Il2CppMethodPointer methodPointer;
    void* invoker_method;
    const char* name;
    hackmatch::il2cpp::Class* klass;
    const void* return_type;
    const void* parameters;
    const void* rgctx_data;
    const void* generic_method;
    std::uint32_t token;
    std::uint16_t flags;
    std::uint16_t iflags;
    std::uint16_t slot;
    std::uint8_t parameters_count;
    std::uint8_t bitflags;
};

namespace hackmatch::il2cpp {

struct Object {
    Class* klass;
    void* monitor;
};

struct ArrayBounds {
    std::uintptr_t length;
    std::int32_t lower_bound;
};

struct Array : Object {
    ArrayBounds* bounds;
    std::uintptr_t max_length;
    Object* vector[1];

    std::uintptr_t size() const { return bounds ? bounds->length : max_length; }
    Object* at(std::uintptr_t index) const { return index < size() ? vector[index] : nullptr; }
};

bool init();
bool ready();
const char* last_error();

Domain* domain();
void attach_thread();
Image* image(const char* assembly_name);
Class* klass(const char* assembly_name, const char* namespaze, const char* class_name);
MethodInfo* method(Class* klass, const char* method_name, int arg_count = -1);
MethodInfo* method(const char* assembly_name, const char* namespaze, const char* class_name, const char* method_name, int arg_count = -1);
FieldInfo* field(Class* klass, const char* field_name);
int field_offset(FieldInfo* field);
Object* type_object(Class* klass);
Array* objects_of_type(Class* klass);
bool read_field(Object* object, FieldInfo* field, void* value);
bool write_field(Object* object, FieldInfo* field, const void* value);
bool read_static_field(FieldInfo* field, void* value);
bool write_static_field(FieldInfo* field, const void* value);
Object* runtime_invoke(MethodInfo* method, void* object, void** args, Object** exception);
void* icall(const char* name);

template <class T>
T method_pointer(MethodInfo* method)
{
    return method ? reinterpret_cast<T>(method->methodPointer) : nullptr;
}
}
