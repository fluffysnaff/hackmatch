#pragma once

#include "hm_enums.h"

class HMConfig
{
public:
	void Hotkeys();

	struct Menu
	{
		bool enabled{ false };
		bool injected{ true };
		int keyEnable{ VK_INSERT };
		int keyUnload{ VK_END };
	} menu;

	struct NoSpread
	{
		bool enabled{ false };
		int key{ VK_F1 };
	} noSpread;

	struct InfAmmo
	{
		bool enabled{ false };
		int key{ VK_F3 };
	} infAmmo;

	struct FastMeele
	{
		bool enabled{ false };
		int key{ VK_F4 };
	} fastMeele;

	struct NoCameraShake
	{
		bool enabled{ false };
	} noCameraShake;

	struct RapidFire
	{
		bool enabled{ false };
		float useDelayRifle{ 0.05f };
		float useDelayOther{ 0.3f };
		int key{ VK_F5 };
		bool gear{ false };
	} rapidFire;

	struct CustomDamage
	{
		bool enabled{ false };
		bool useGlobal{ true };
		DamageLobyDataEnum global{ DamageLobyDataEnum::RS_SniperDamage };
		DamageLobyDataEnum rifle{ DamageLobyDataEnum::RS_RifleDamage };
		DamageLobyDataEnum shotgun{ DamageLobyDataEnum::RS_ShotgunDamage };
		DamageLobyDataEnum sniper{ DamageLobyDataEnum::RS_SniperDamage };
		DamageLobyDataEnum revolver{ DamageLobyDataEnum::RS_RevolverDamage };
		int key{ VK_F6 };
		bool gear{ false };
	} customDamage;

	struct FlyHack
	{
		bool enabled{ false };
		bool flying{ false };
		int key{ VK_LSHIFT };
	} flyHack;

	struct PlayerEsp
	{
		bool enabled{ false };
		bool nameplates{ false };
		bool box{ false };
	} playerEsp;

	struct AntiShielded
	{
		bool enabled{ false };
	} antiShielded;
};