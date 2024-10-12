#pragma once

#include <iostream>
#include <windows.h>
#include <vector>
#include <string>
#include <map>

#include "../imgui/imgui.h"
#include "il2cpp.h"
#include "structures.h"

struct Vector3
{
	float x, y, z;
	Vector3(UnityEngine_Vector3_o ptr) { this->x = ptr.fields.x; this->y = ptr.fields.y; this->z = ptr.fields.z; };
	Vector3(float _x, float _y, float _z) { this->x = _x; this->y = _y; this->z = _z; };
};

class IL2CppHelper
{
public:
	IL2CppHelper();

	template <typename ret, typename... _args>
	ret call_function(MethodInfo* method, _args... args);

	il2cpp::il2cppAssembly* GetAssembly(const char* targetIl2cppAssembly);
	il2cpp::il2cppImage* GetImage(const char* targetIl2cppAssembly);

	il2cpp::il2cppClass* find_class(const char* namespxce, const char* class_name);
	il2cpp::il2cppClass* find_class(il2cpp::il2cppAssembly* assembly, const char* namespxce, const char* class_name);
	MethodInfo* get_fn_ptr(il2cpp::il2cppClass* klass, const char* _method, int args_count = 0);
	MethodInfo* get_method(const char* namespxce, const char* class_name, const char* method);
	MethodInfo* get_method(il2cpp::il2cppAssembly* assembly, const char* namespxce, const char* class_name, const char* method);
	std::string to_string(System_String_o* str);

	template <class type>
	type get_static_field_value(il2cpp::il2cppClass* klass, const char* field_name)
	{
		auto field = il2cpp::field_from_name(klass, field_name);
		type buffer;
		il2cpp::static_field_get_value(field, &buffer);
		return buffer;
	}

	template <class type>
	void set_static_field_value(il2cpp::il2cppClass* klass, const char* field_name, type val)
	{
		auto field = il2cpp::field_from_name(klass, field_name);
		il2cpp::static_field_set_value(field, &val);
	}

	UnityEngine_Camera_o* camera_get_main();
	bool world_to_screen(Vector3 world, ImVec2& out);
	void transform_look_at(UnityEngine_Transform_o* transform, UnityEngine_Vector3_o pos);
	UnityEngine_Vector3_o rigidbody_get_velocity(UnityEngine_Rigidbody_o* body);

	void PrintAssemblyMap();

private:
	std::map<const char*, il2cpp::il2cppAssembly*> assemblyMap;
};