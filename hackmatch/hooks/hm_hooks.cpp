#include <Windows.h>
#include "../kiero/minhook/include/MinHook.h"
#include "../il2cpp/il2cpp.h"
#include "../hm_helper.h"

#include "hm_hooks.h"

bool HMHooks::HookFunction(void* funPtr, void* callbackFun)
{
	if (!funPtr || !callbackFun)
		return false;

	HookedFunction fn;
	fn.callback = callbackFun;
	fn.target = funPtr;

	MH_CreateHook((uintptr_t*)funPtr, callbackFun, (LPVOID*)&fn.orig);
	hooks.push_back(fn);
	return MH_EnableHook(funPtr) == MH_OK;
}

void __cdecl hk_controller_update(PlayerController_o* thisPtr)
{
	// Update sdk players
	hmHelper->m_pSdk->UpdatePlayer(thisPtr);

	// Call hacks
	hmHelper->m_pHacks->RunHacks(thisPtr);

	return reinterpret_cast<decltype(&hk_controller_update)>(hmHelper->m_pHooks->hooks[0].orig)(thisPtr);
}

void HMHooks::Init()
{
	if (!HookFunction(hmHelper->m_pIl2cpp->get_method("", "PlayerController", "Update")->methodPointer, hk_controller_update))
		ASSERT("Couldn't initialize hook: PlayerController->Update()")
}
