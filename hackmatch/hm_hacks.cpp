#include "hm_hacks.h"
#include "il2cpp/il2cpp.h"
#include "hm_enums.h"
#include "hm_helper.h"


// Main
void HMHacks::RunHacks(PlayerController_o* thisPtr)
{
    ACBypass();
    NoSpread();
    InfAmmo();
    FastMeele();
    NoCameraShake();
    RapidFire();
    CustomDamage();
    FlyHack(thisPtr);
    AntiShielded(thisPtr);
}

bool HMHacks::SaveFields()
{
    PlayerController_o* player = hmHelper->m_pSdk->GetLocalPlayer();
    if (!player)
        return false;
    for (int i = 0; i < static_cast<int>(player->fields.items->max_length); i++)
    {
        const Item_o* item = player->fields.items->m_Items[i];
        if (!item)
            continue;
        savedItemFields.push_back(item->fields.info->fields);
    }
    return true;
}


// Hacks
void HMHacks::NoSpread()
{
    PlayerController_o* player = hmHelper->m_pSdk->GetLocalPlayer();
    if (!player)
        return;

    static bool noSpreadState = false;
    if (!hmHelper->m_pConfig->noSpread.enabled)
    {
        if (noSpreadState)
        {
            for (int i = 0; i < static_cast<int>(player->fields.items->max_length); i++)
            {
            	Item_o* item = player->fields.items->m_Items[i];
                if (!item)
                    continue;
                if (i > savedItemFields.size())
                    break;
                item->fields.info->fields.adsBulletSpread = savedItemFields.at(i).adsBulletSpread;
                item->fields.info->fields.adsMovementSpreadMultiplier = savedItemFields.at(i).adsMovementSpreadMultiplier;
                item->fields.info->fields.adsSpread = savedItemFields.at(i).adsSpread;
                item->fields.info->fields.bulletSpread = savedItemFields.at(i).bulletSpread;
                item->fields.info->fields.normalSpread = savedItemFields.at(i).normalSpread;
                item->fields.info->fields.bulletSpreadDecrease = savedItemFields.at(i).bulletSpreadDecrease;
                item->fields.info->fields.maxBulletSpread = savedItemFields.at(i).maxBulletSpread;
                item->fields.info->fields.movementSpreadMultiplier = savedItemFields.at(i).movementSpreadMultiplier;
            }
            noSpreadState = false;
        }
        return;
    }

    if (!player->fields.items)
        return;
    
    // Disables normal bulletSpread
    player->fields.bulletSpread = 0.f;

    for (int i = 0; i < static_cast<int>(player->fields.items->max_length); i++)
    {
        const Item_o* item = player->fields.items->m_Items[i];
        if (!item)
            continue;
        item->fields.info->fields.adsBulletSpread = 0.f;
        item->fields.info->fields.adsMovementSpreadMultiplier = 0.f;
        item->fields.info->fields.adsSpread = 0.f;
        item->fields.info->fields.bulletSpread = 0.f;
        item->fields.info->fields.normalSpread = 0.f;
        item->fields.info->fields.bulletSpreadDecrease = 0.f;
        item->fields.info->fields.maxBulletSpread = 0.f;
        item->fields.info->fields.movementSpreadMultiplier = 0.f;
    }
    noSpreadState = true;
}

void HMHacks::InfAmmo()
{
    if (!hmHelper->m_pConfig->infAmmo.enabled)
        return;

    PlayerController_o* player = hmHelper->m_pSdk->GetLocalPlayer();
    if (!player)
        return;
    if (!player->fields.items)
        return;

    for (int i = 0; i < static_cast<int>(player->fields.items->max_length); i++)
    {
    	Item_o* item = player->fields.items->m_Items[i];
        if (!item)
            continue;

        // Idk what this is
        item->fields.ammo.fields.offset = 0;
        item->fields.totalAmmo.fields.offset = 0;

        // Set ammo to 9999
        item->fields.ammo.fields.value = 9999;
        item->fields.totalAmmo.fields.value = 9999;
    }
}

void HMHacks::ACBypass()
{
    // Anti-Anti cheat
    PlayerController_o* player = hmHelper->m_pSdk->GetLocalPlayer();
    if (!player)
        return;

    // Disable offenses and fireTimeDifference stuff
    player->fields.antiCheat_offenses = 0;
    player->fields.antiCheat_lastFireTime = -1;
    player->fields.fireTimeDifference->fields._average_k__BackingField = 999.f;
    player->fields.fireTimeDifference->fields.sampleAccumulator = 999.f;

    // Remove all samples from fireTimeDifference
	const System_Collections_Generic_Queue_float__o* samples = player->fields.fireTimeDifference->fields.samples;
    for (int i = 0; i < samples->fields._size; i++)
        samples->fields._array->m_Items[i] = 999.f;

    // QOL features
    if (!player->fields.items)
        return;
    for (int i = 0; i < static_cast<int>(player->fields.items->max_length); i++)
    {
    	Item_o* item = player->fields.items->m_Items[i];
        if (!item)
            continue;

    	if (hmHelper->m_pConfig->noSpread.enabled)
        {
            // On no spread -> force scope in kill log(less sus)
            item->fields.info->fields.noscope = false;

            // If OP shotgun enabled -> force sniper in kill logs(so so much less sus)
            if (item->fields.info->fields.damageData == static_cast<int32_t>(DamageDataEnum::shotgun))
                item->fields.info->fields.damageData = static_cast<int32_t>(DamageDataEnum::sniper);
        }
    }
}

void HMHacks::FastMeele()
{
    if (!hmHelper->m_pConfig->fastMeele.enabled)
        return;

    PlayerController_o* player = hmHelper->m_pSdk->GetLocalPlayer();
    if (!player)
        return;

    player->fields.meleeing = false;
}

void HMHacks::NoCameraShake()
{
    PlayerController_o* player = hmHelper->m_pSdk->GetLocalPlayer();
    if (!player)
        return;

    static bool noCameraShakeState = false;
    if (!hmHelper->m_pConfig->noCameraShake.enabled)
    {
        if (noCameraShakeState)
        {
            for (int i = 0; i < static_cast<int>(player->fields.items->max_length); i++)
            {
                Item_o* item = player->fields.items->m_Items[i];
                if (!item)
                    continue;
                if (i > savedItemFields.size())
                    break;

                item->fields.info->fields.cameraShake = savedItemFields.at(i).cameraShake;
                item->fields.info->fields.adsCameraShake = savedItemFields.at(i).adsCameraShake;
                item->fields.info->fields.cameraADSBobMultiplier = savedItemFields.at(i).cameraADSBobMultiplier;
            }
            noCameraShakeState = false;
        }
        return;
    }

    if (!player->fields.items)
        return;

    for (int i = 0; i < static_cast<int>(player->fields.items->max_length); i++)
    {
        Item_o* item = player->fields.items->m_Items[i];
        if (!item)
            continue;

        item->fields.info->fields.cameraShake = 0.f;
        item->fields.info->fields.adsCameraShake = 0.f;
        item->fields.info->fields.cameraADSBobMultiplier = 0.f;
    }
    noCameraShakeState = true;
}

void HMHacks::RapidFire()
{
    PlayerController_o* player = hmHelper->m_pSdk->GetLocalPlayer();
    if (!player)
        return;

    static bool rapidFireState = false;
    if (!hmHelper->m_pConfig->rapidFire.enabled)
    {
        if (rapidFireState)
        {
            for (int i = 0; i < static_cast<int>(player->fields.items->max_length); i++)
            {
                Item_o* item = player->fields.items->m_Items[i];
                if (!item)
                    continue;
                if (i > savedItemFields.size())
                    break;
                item->fields.info->fields.useDelay = savedItemFields.at(i).useDelay;
            }
            rapidFireState = false;
        }
        return;
    }

    if (!player->fields.items)
        return;

    for (int i = 0; i < static_cast<int>(player->fields.items->max_length); i++)
    {
        const Item_o* item = player->fields.items->m_Items[i];
        if (!item)
            continue;
        // Check if current weapon is rifle, then use a lower delay otherwise use higher delay
        if (item->fields.info->fields.damageData == static_cast<int32_t>(DamageDataEnum::rifle))
        	item->fields.info->fields.useDelay = hmHelper->m_pConfig->rapidFire.useDelayRifle;
        else
            item->fields.info->fields.useDelay = hmHelper->m_pConfig->rapidFire.useDelayOther;
    }
    rapidFireState = true;
}

void HMHacks::CustomDamage()
{
    PlayerController_o* player = hmHelper->m_pSdk->GetLocalPlayer();
    if (!player)
        return;

    static bool customDamageState = false;
    if (!hmHelper->m_pConfig->customDamage.enabled)
    {
        if (customDamageState)
        {
            for (int i = 0; i < static_cast<int>(player->fields.items->max_length); i++)
            {
                Item_o* item = player->fields.items->m_Items[i];
                if (!item)
                    continue;
                if (i > savedItemFields.size())
                    break;
                item->fields.info->fields.damageLobbyData = savedItemFields.at(i).damageLobbyData;
            }
            customDamageState = false;
        }
        return;
    }

    if (!player->fields.items)
        return;

    for (int i = 0; i < static_cast<int>(player->fields.items->max_length); i++)
    {
        const Item_o* item = player->fields.items->m_Items[i];
        if (!item)
            continue;
        if (hmHelper->m_pConfig->customDamage.useGlobal)
			item->fields.info->fields.damageLobbyData = static_cast<int32_t>(hmHelper->m_pConfig->customDamage.global);
        else
        {
            if(item->fields.info->fields.damageData == static_cast<int32_t>(DamageDataEnum::rifle))
                item->fields.info->fields.damageLobbyData = static_cast<int32_t>(hmHelper->m_pConfig->customDamage.rifle);
            else if (item->fields.info->fields.damageData == static_cast<int32_t>(DamageDataEnum::shotgun))
                item->fields.info->fields.damageLobbyData = static_cast<int32_t>(hmHelper->m_pConfig->customDamage.shotgun);
            else if (item->fields.info->fields.damageData == static_cast<int32_t>(DamageDataEnum::sniper))
                item->fields.info->fields.damageLobbyData = static_cast<int32_t>(hmHelper->m_pConfig->customDamage.sniper);
            else if (item->fields.info->fields.damageData == static_cast<int32_t>(DamageDataEnum::revolver))
                item->fields.info->fields.damageLobbyData = static_cast<int32_t>(hmHelper->m_pConfig->customDamage.revolver);
        }
    }
    customDamageState = true;
}

void HMHacks::FlyHack(PlayerController_o* thisPtr)
{
    PlayerController_o* player = hmHelper->m_pSdk->GetLocalPlayer();
    if (!player || !thisPtr)
        return;

    static bool flyHackState = false;
    if (!hmHelper->m_pConfig->flyHack.enabled || !hmHelper->m_pConfig->flyHack.flying)
    {
        if (flyHackState)
        {
            player->fields.toJump = false;
            flyHackState = false;
        }
        return;
    }

    if (player != thisPtr)
        return;

	if(hmHelper->m_pConfig->flyHack.flying)
        player->fields.toJump = true;

    flyHackState = true;
}

void HMHacks::AntiShielded(PlayerController_o* thisPtr)
{
    PlayerController_o* player = hmHelper->m_pSdk->GetLocalPlayer();
    if (!player || !thisPtr)
        return;

    if (!hmHelper->m_pConfig->antiShielded.enabled)
        return;

    if (player == thisPtr)
        return;

    thisPtr->fields.shielded = false;
}
