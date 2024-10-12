#include "hm_helper.h"

HMHelper::HMHelper() :
	m_pIl2cpp(std::make_unique<IL2CppHelper>()),
	m_pHacks(std::make_unique<HMHacks>()),
	m_pConfig(std::make_unique<HMConfig>()),
	m_pGui(std::make_unique<HMGui>()),
	m_pHooks(std::make_unique<HMHooks>()),
	m_pSdk(std::make_unique<HMSdk>()),
	m_pEsp(std::make_unique<HMEsp>()) {}

int HMHelper::GetIdxFromDmgTypeSelection(DamageLobyDataEnum selection)
{
	if (selection == DamageLobyDataEnum::RS_RifleDamage)
		return 0;
	if (selection == DamageLobyDataEnum::RS_ShotgunDamage)
		return 1;
	if (selection == DamageLobyDataEnum::RS_SniperDamage)
		return 2;
	if (selection == DamageLobyDataEnum::RS_RevolverDamage)
		return 3;
	return 0;
}

DamageLobyDataEnum HMHelper::GetDmgTypeFromIdx(int selection)
{
	if (selection == 0)
		return DamageLobyDataEnum::RS_RifleDamage;
	if (selection == 1)
		return DamageLobyDataEnum::RS_ShotgunDamage;
	if (selection == 2)
		return DamageLobyDataEnum::RS_SniperDamage;
	if (selection == 3)
		return DamageLobyDataEnum::RS_RevolverDamage;
	return DamageLobyDataEnum::RS_RifleDamage;
}
