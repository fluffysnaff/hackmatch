#include "hm_il2cpp_helper.h"

#define ASSERT(x) { MessageBoxA(NULL, NULL, x, NULL); exit(0); }

IL2CppHelper::IL2CppHelper()
{
	if (!il2cpp::resolve_call)
		ASSERT("Failed to get resolve call.")
	if (!il2cpp::get_domain)
		ASSERT("Failed to get get domain.")
	if (!il2cpp::thread_attach)
		ASSERT("Failed to get thread attach")

	bool foundMainAssembly = false;
	size_t size = 0;
	il2cpp::il2cppAssembly** assemblies = il2cpp::get_assemblies(il2cpp::get_domain(), &size);
	for (size_t i = 0; i < size; i++)
	{
		il2cpp::il2cppAssembly* assembly = assemblies[i];
		if (!assembly)
			continue;

		// Create a map with assembly name and the il2cppAssembly;
		const char* assemblyName = assembly->m_aName.m_pName;
		assemblyMap[assemblyName] = assembly;

		// Check if we found the main assembly
		if (std::string(assemblyName) == "Assembly-CSharp")
			foundMainAssembly = true;
	}
	
	if (!foundMainAssembly)
		ASSERT("Failed to find assembly.")
}

template <typename ret, typename... _args>
ret IL2CppHelper::call_function(MethodInfo* method, _args... args)
{
	typedef ret(*func)(_args...);
	func fn = (func)((void*)method->methodPointer);
	return fn(args...);
}

il2cpp::il2cppAssembly* IL2CppHelper::GetAssembly(const char* targetIl2cppAssembly)
{
	for (auto& entry : assemblyMap)
		if (strcmp(entry.first, targetIl2cppAssembly) == 0)
			return entry.second;
	return nullptr;
}

il2cpp::il2cppImage* IL2CppHelper::GetImage(const char* targetIl2cppAssembly)
{
	for (auto& entry : assemblyMap)
		if (strcmp(entry.first, targetIl2cppAssembly) == 0)
			return entry.second->m_pImage;
	return nullptr;
}

il2cpp::il2cppClass* IL2CppHelper::find_class(const char* namespxce, const char* class_name)
{
	return il2cpp::class_from_name(GetImage("Assembly-CSharp"), namespxce, class_name);
}

il2cpp::il2cppClass* IL2CppHelper::find_class(il2cpp::il2cppAssembly* assembly, const char* namespxce, const char* class_name)
{
	return il2cpp::class_from_name(assembly->m_pImage, namespxce, class_name);
}

MethodInfo* IL2CppHelper::get_fn_ptr(il2cpp::il2cppClass* klass, const char* _method, int args_count)
{
	return il2cpp::method_from_name(klass, _method, args_count);
}

MethodInfo* IL2CppHelper::get_method(const char* namespxce, const char* class_name, const char* method)
{
	auto klass = find_class(namespxce, class_name);

	for (int i = 0; i < 16; i++)
	{
		auto info = il2cpp::method_from_name(klass, method, i);
		if (!info)
			continue;

		return info;
	}
	return nullptr;
}

MethodInfo* IL2CppHelper::get_method(il2cpp::il2cppAssembly* assembly, const char* namespxce, const char* class_name, const char* method)
{
	auto klass = find_class(assembly, namespxce, class_name);

	for (int i = 0; i < 16; i++)
	{
		auto info = il2cpp::method_from_name(klass, method, i);
		if (!info)
			continue;

		return info;
	}

	return nullptr;
}

std::string IL2CppHelper::to_string(System_String_o* str)
{
	if (!str)
		return "NULL";
	
	auto wstr = std::wstring((wchar_t*)(&str->fields.m_firstChar));
	return std::string(wstr.begin(), wstr.end());
}

UnityEngine_Camera_o* IL2CppHelper::camera_get_main()
{
	static auto camera_get_main = reinterpret_cast<UnityEngine_Camera_o * (__fastcall*)(void)>(il2cpp::resolve_call("UnityEngine.Camera::get_main()"));
	return camera_get_main();
}

bool IL2CppHelper::world_to_screen(Vector3 world, ImVec2& out)
{

	static il2cpp::il2cppAssembly* coremodule = nullptr;
	if (!coremodule)
	{
		coremodule = GetAssembly("UnityEngine.CoreModule");
		if (!coremodule)
			return false;
	}
	static auto world_to_screen_point = reinterpret_cast<Vector3(__cdecl*)(UnityEngine_Camera_o*, Vector3)>(get_method(coremodule, "UnityEngine", "Camera", "WorldToScreenPoint")->methodPointer);
	UnityEngine_Camera_o* camera = camera_get_main();
	if (!camera)
		return false;
	Vector3 pos = world_to_screen_point(camera, world);
	if (pos.z < 0.f)
		return false;
	ImVec2 screen_size = ImGui::GetIO().DisplaySize;
	out = ImVec2{ pos.x, screen_size.y - pos.y };
	return true;
}

void IL2CppHelper::transform_look_at(UnityEngine_Transform_o* transform, UnityEngine_Vector3_o pos)
{
	static il2cpp::il2cppAssembly* coremodule = nullptr;
	if (!coremodule)
	{
		coremodule = GetAssembly("UnityEngine.CoreModule");
		if (!coremodule)
			return;
	}
	uintptr_t gameassembly = (uintptr_t)GetModuleHandleA("GameAssembly.dll");
	static auto transform_look_at = reinterpret_cast<void(__cdecl*)(UnityEngine_Transform_o*, UnityEngine_Vector3_o)>(gameassembly + 0xDDE2B0);
	return transform_look_at(transform, pos);
}

UnityEngine_Vector3_o IL2CppHelper::rigidbody_get_velocity(UnityEngine_Rigidbody_o* body)
{
	static il2cpp::il2cppAssembly* physicsmodule = nullptr;
	if (!physicsmodule)
	{
		physicsmodule = GetAssembly("UnityEngine.PhysicsModule");
		if (!physicsmodule)
			return UnityEngine_Vector3_o{0, 0, 0};
	}
	static auto rigidbody_get_velocity = reinterpret_cast<UnityEngine_Vector3_o(__cdecl*)(UnityEngine_Rigidbody_o*)>(get_method(physicsmodule, "UnityEngine", "RigidBody", "get_velocity")->methodPointer);
	return rigidbody_get_velocity(body);
}

void IL2CppHelper::PrintAssemblyMap()
{
	if (!assemblyMap.empty())
		for (const auto& entry : assemblyMap)
			std::cout << entry.first << ": " << entry.second << std::endl;
}
