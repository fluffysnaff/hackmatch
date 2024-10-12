#include <cstdio>
#include <iostream>
#define WIN32_LEAN_AND_MEAN
#define NO_MIN_MAX

#include "hm_global_vars.h"
#include "il2cpp/il2cpp.h"
#include "hm_version.h"

#include "hm_helper.h"
std::unique_ptr<HMHelper> hmHelper;
std::unique_ptr<HMVariables> vars;

DWORD WINAPI MainThread(HMODULE hmodule)
{
	bool init_hook = false;
	FILE* file = nullptr;
	do
	{
		if (kiero::init(kiero::RenderType::D3D11) == kiero::Status::Success)
		{
			AllocConsole();
			SetConsoleTitleA(APP_NAME);
			freopen_s(&file, "CONOUT$", "w", stdout);

			 // Initialize hmHelper
			hmHelper = std::make_unique<HMHelper>();
			vars = std::make_unique<HMVariables>();
			hmHelper->m_pHooks->Init();

			// Bind kiero
			kiero::bind(8, (void**)&hmHelper->m_pGui->oPresent, hmHelper->m_pGui->hkPresent);
#if _DEBUG
			// Print assemblies(for debugging)
			hmHelper->m_pIl2cpp->PrintAssemblyMap();
#endif
			std::cout << APP_NAME << " successfully initialized...\n";
			std::cout << "Press INS to open window!\n";

			init_hook = true;
		}
	} while (!init_hook);

	// Handle unloading
	while (hmHelper->m_pConfig->menu.injected) 
	{
		if (GetAsyncKeyState(hmHelper->m_pConfig->menu.keyUnload) & 1)
		{
			if(file != nullptr)
				fclose(file);
			FreeConsole();

			hmHelper->m_pConfig->menu.injected = false;
			Sleep(1000);
			FreeLibraryAndExitThread(hmodule, 0);
		}
	}

	return TRUE;
}

BOOL WINAPI DllMain(HMODULE hMod, DWORD dwReason, LPVOID lpReserved)
{
	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(hMod);
		CloseHandle(CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)MainThread, hMod, 0, nullptr));
		break;
	case DLL_PROCESS_DETACH:
		kiero::shutdown();
		break;
	default:
		break;
	}
	return TRUE;
}