#include "../hm_helper.h"
#include "hm_sdk.h"

PlayerController_o* HMSdk::GetLocalPlayer()
{
	return hmHelper->m_pIl2cpp->get_static_field_value<PlayerController_o*>(hmHelper->m_pIl2cpp->find_class("", "PlayerController"), "LocalInstance");
}

void HMSdk::UpdatePlayer(PlayerController_o* thisptr)
{
	PlayerController_o* localPlayer = GetLocalPlayer();
	if (localPlayer == nullptr)
		return;
	if (!thisptr->fields._identity_k__BackingField->fields.player || thisptr == localPlayer)
		return;

	// Check if player is already in the players list
	for (const auto& player : players)
	{
		if (player == nullptr)
			continue;
		if (player == thisptr)
			return;
	}

	// Using mutex cuz it crashed before
	playersMutex.lock();
	players.push_back(thisptr);
	playersMutex.unlock();
}
