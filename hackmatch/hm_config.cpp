#include <windows.h>
#include <iostream>
#include "hm_config.h"

#include "hm_helper.h"

void HMConfig::Hotkeys()
{
	if (GetAsyncKeyState(menu.keyEnable) & 1)
	{
		hmHelper->m_pGui->animLoad = 0.f;
		menu.enabled = !menu.enabled;
	}
	else if (GetAsyncKeyState(noSpread.key) & 1)
	{
		noSpread.enabled = !noSpread.enabled;
	}
	else if (GetAsyncKeyState(infAmmo.key) & 1)
	{
		infAmmo.enabled = !infAmmo.enabled;
	}
	else if (GetAsyncKeyState(fastMeele.key) & 1)
	{
		fastMeele.enabled = !fastMeele.enabled;
	}
	else if (GetAsyncKeyState(rapidFire.key) & 1)
	{
		rapidFire.enabled = !rapidFire.enabled;
	}
	else if (GetAsyncKeyState(customDamage.key) & 1)
	{
		customDamage.enabled = !customDamage.enabled;
	}

	// Flyhack
	if ((GetAsyncKeyState(flyHack.key) & 0x8000) != 0)
	{
		flyHack.flying = !flyHack.flying;
	}
}
