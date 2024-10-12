#pragma once

#include <vector>
#include "il2cpp/il2cpp.h"

class HMHacks
{
public:
	// Main
	void RunHacks(PlayerController_o* thisPtr);

	std::vector<ItemInfo_Fields> savedItemFields;
	bool SaveFields();

private:
	// Hacks
	void NoSpread();
	void InfAmmo();
	void ACBypass();
	void FastMeele();
	void NoCameraShake();
	void RapidFire();
	void CustomDamage();

	// Hook hacks
	void FlyHack(PlayerController_o* thisPtr);
	void AntiShielded(PlayerController_o* thisPtr);
};
