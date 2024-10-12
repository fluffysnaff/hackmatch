#include "../hm_helper.h"
#include "hm_esp.h"

// Main
void HMEsp::RenderEsp()
{
	if (!hmHelper->m_pConfig->playerEsp.enabled)
		return;
	RenderNameplates();
	RenderBox();
}


// Rendering
void HMEsp::RenderNameplates()
{
	if (!hmHelper->m_pConfig->playerEsp.nameplates)
		return;

	PlayerController_o* localPlayer = hmHelper->m_pSdk->GetLocalPlayer();
	if (localPlayer == nullptr)
		return;

	hmHelper->m_pSdk->playersMutex.lock();
	for (const auto& player : hmHelper->m_pSdk->players)
	{
		if (!player)
			continue;
		ImVec2 screen;

		if (!hmHelper->m_pIl2cpp->world_to_screen(player->fields.lastNonLocalPos, screen))
			continue;

		std::string name = hmHelper->m_pIl2cpp->to_string(player->fields._identity_k__BackingField->fields._Owner_k__BackingField->fields._name);
		screen.x -= ImGui::CalcTextSize(name.c_str()).x * 0.5f;
		ImGui::GetBackgroundDrawList()->AddText(screen, IM_COL32(255, 0, 0, 255), name.c_str());

	}
	hmHelper->m_pSdk->playersMutex.unlock();
}

void HMEsp::RenderBox()
{
	if (!hmHelper->m_pConfig->playerEsp.box)
		return;

	PlayerController_o* localPlayer = hmHelper->m_pSdk->GetLocalPlayer();
	if (localPlayer == nullptr)
		return;

	hmHelper->m_pSdk->playersMutex.lock();
	for (const auto& player : hmHelper->m_pSdk->players)
	{
		if (!player)
			continue;

		constexpr float appxHeight = 2.f;
		UnityEngine_Vector3_o pos = player->fields.lastNonLocalPos;

		UnityEngine_Vector3_o feet = { pos.fields.x, pos.fields.y - appxHeight * 0.5f, pos.fields.z };
		UnityEngine_Vector3_o head = { pos.fields.x, pos.fields.y + appxHeight * 0.5f, pos.fields.z };

		ImVec2 tmpHead;
		if (!hmHelper->m_pIl2cpp->world_to_screen(head, tmpHead))
			continue;

		ImVec2 tmpFeet;
		if (!hmHelper->m_pIl2cpp->world_to_screen(feet, tmpFeet))
			continue;

		const float height = GetPlayerHeight(tmpHead, tmpFeet);
		const float width = height * 0.5f;

		const ImVec2 min = { tmpFeet.x - width * 0.5f, tmpFeet.y - height };		   // upper-left
		const ImVec2 max = { tmpFeet.x + width * 0.5f, tmpFeet.y };                     // lower-right

		ImVec2 screen;
		if (!hmHelper->m_pIl2cpp->world_to_screen(player->fields.lastNonLocalPos, screen))
			continue;

		ImGui::GetBackgroundDrawList()->AddRect(min,
			max,
			ColorRGBA{ 0.f, 0.f, 0.f, 225.f }.imGui(),
			0.f,
			ImDrawCornerFlags_All,
			2.5f);
		ImGui::GetBackgroundDrawList()->AddRect(min, max, IM_COL32(255, 0, 0, 255), 0.f,ImDrawCornerFlags_All, 1.5f);
	}
	hmHelper->m_pSdk->playersMutex.unlock();
}
