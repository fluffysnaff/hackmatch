#include "../hm_helper.h"
#include "../data/fonts/fa.h"
#include "../hm_global_vars.h"
#include "../hm_version.h"

#include "hm_gui.h"


// Menu
void HMGui::RenderMainWindow()
{
	if (!hmHelper->m_pConfig->menu.enabled)
		return;

	ImVec2 windowSize = ImVec2(800, 600);
	std::string windowName = "##mainWindow";
	bool* windowOpen = &hmHelper->m_pConfig->menu.enabled;
	ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysUseWindowPadding;

	// Constants
	static ImVec2 actualWinSize = windowSize;
	constexpr float selectableOffset = 30.f;
	const auto leftSideSize = ImVec2(windowSize.x * 0.25f, windowSize.y);
	const ImVec2 selectableSize = ImVec2(leftSideSize.x - selectableOffset, 28.f);

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0)); // Remove free space around the window
	ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(0.f, 0.f, 0.f, 0.f)); // Transparent window bg

	// Set first time window size
	ImGui::SetNextWindowSize(windowSize, ImGuiCond_Once);

	ImGui::Begin(windowName.c_str(), windowOpen, windowFlags);

	// Use accurate window size
	actualWinSize = ImGui::GetWindowSize();
	actualWinSize = ImVec2(IM_ROUND(actualWinSize.x), IM_ROUND(actualWinSize.y));

	// Note: Debug tab must be on the end!
	const std::vector tabNames = { "Main", "Esp" };
	const std::vector tabIcons = { ICON_FA_SCREWDRIVER_WRENCH, ICON_FA_USER };

	// Define tab count, if it's debug mode increase the count
	int tabCount = static_cast<int>(tabNames.size());
#ifdef _DEBUG
	tabCount += 1;
#endif
	static int tab = 0;

	// Set opacity
	const float opacity = 255.f * 0.85f;
	Colors::tabsBgColor.a = opacity;

	// Left side
	ImGui::PushStyleColor(ImGuiCol_Text, Colors::White.imGui());
	ImGui::PushStyleColor(ImGuiCol_ChildBg, Colors::tabsBgColor.imGui());
	ImGui::SetCursorPosY(0.f);
	ImGui::BeginChild("##left", leftSideSize, true, ImGuiWindowFlags_AlwaysUseWindowPadding);
	{
		hmHelper->m_pGui->animLoad = ImLerp(hmHelper->m_pGui->animLoad, 1.f, 0.045f);
		// Show logo
		const auto logoSize = ImVec2(64.f, 64.f);
		// const ImVec2 logoPos = ImVec2{ (leftSideSize.x - logoSize.x) * 0.5f, 20.f };
		// hmHelper->m_pGui->DrawZenithLogo(logoPos, logoSize);

		ImGui::Spacing();

		// Draw name
		ImGui::PushFont(hmHelper->m_pGui->titleFont);
		ImGui::SetCursorPosX((leftSideSize.x - ImGui::CalcTextSize(APP_NAME).x) * 0.5f);
		ImGui::Text(APP_NAME);
		ImGui::PopFont();

		// Spacing between logo and "Main"
		ImGui::Spacing();
		ImGui::Spacing();

		// Section "Main"
		ImGui::PushFont(hmHelper->m_pGui->subFont);
		ImGui::PushStyleColor(ImGuiCol_Text, Colors::subFontColor.imGui());
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + selectableOffset * 0.5f + vars->widgetOffset);
		ImGui::Text("Main");
		ImGui::PopStyleColor();
		ImGui::PopFont();

		ImGui::Indent(selectableOffset * 0.5f);
		for (int i = 0; i < tabCount; i++)
		{
			// I don't like ImGui, there is some weird color packing
			Colors::tabsBgColor.a = 0.f;

			const bool selected = tab == i;
			const ImU32 color = selected ? Colors::UltramarineBlue.imGui() : Colors::tabsBgColor.imGui();

			// Disable hover effect + change selectable align
			ImGui::PushStyleColor(ImGuiCol_HeaderHovered, color);
			ImGui::PushStyleColor(ImGuiCol_HeaderActive, color);
			ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.f, 0.5f));
			if (hmHelper->m_pGui->TabButton(tabNames[i], tabIcons[i], ImGuiButtonFlags_None, selectableSize, color) && tab != i)
			{
				tab = i;
				hmHelper->m_pGui->animLoad = 0.f;
			}
			ImGui::PopStyleVar();
			ImGui::PopStyleColor(2); // header hovered, header active

			if (selected)
				ImGui::SetItemDefaultFocus();

			// Spacing between selectables
			ImGui::Spacing();
			ImGui::Spacing();
		}
		// Render settings
		// ImGui::SetCursorPos(ImVec2(selectableOffset * 0.5f, leftSideSize.y - (selectableOffset * 0.5f + 25.f)));
		// const ImU32 color = manager->config->menu.settingsWindow ? Colors::UltramarineBlue.imGui() : Colors::White.imGui();
		// ImGui::PushStyleColor(ImGuiCol_Text, color);
		// if (manager->gui->TabButton("##settingsButton", ICON_FA_GEAR, ImGuiButtonFlags_None, ImVec2(25.f, 25.f), Colors::tabsBgColor.imGui()))
		// 	manager->config->menu.settingsWindow = !manager->config->menu.settingsWindow;
		// ImGui::PopStyleColor();
		Colors::tabsBgColor.a = opacity;
	}
	ImGui::PopStyleColor(2); // text, childbg

	ImGui::Unindent();
	ImGui::EndChild();

	ImGui::SameLine();

	// Right side
	ImGui::PushStyleColor(ImGuiCol_ChildBg, Colors::Xiketic.imGui());
	ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX() - 9.f, 0.f)); // Hacky way around evil free space
	ImGui::BeginChild(
		"##right",
		ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y),
		true,
		ImGuiWindowFlags_AlwaysUseWindowPadding | ImGuiWindowFlags_NoScrollbar);
	{
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, hmHelper->m_pGui->animLoad);
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 8.f - 8.f * hmHelper->m_pGui->animLoad); // 8 is arbitrary
		ImGui::Spacing();
		ImGui::Spacing();
		switch (tab)
		{
		case 0:
		{
			hmHelper->m_pGui->RenderMiscTab();
			break;
		}
		case 1:
			hmHelper->m_pGui->RenderEspTab();
			break;
		// case 2:
		// 	manager->gui->RenderObjectsTab();
		// 	break;
		// case 3:
		// 	manager->gui->RenderMiscTab();
		// 	break;
		// case 4:
		// 	manager->gui->RenderConfigTab();
		// 	break;
		default:
			break;
		}
		ImGui::PopStyleVar();
	}
	ImGui::PopStyleColor(2); // window padding, window bg
	ImGui::EndChild();

	ImGui::End();
	ImGui::PopStyleVar();
	Colors::tabsBgColor.a = 255.f;
}

void HMGui::RenderMiscTab()
{
	ImGui::Indent(vars->childWindowOffset);
	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 10.f);
	ImGui::PushStyleColor(ImGuiCol_ChildBg, Colors::tabsBgColor.imGui());
	static float sizeDiffs[4] = { 0.f, 0.f, 0.f, 0.f };
	ImVec2 size = CalculateWindowSize(vars->childWindowOffset);
	size.y = sizeDiffs[0];
	ImGui::BeginChild("##miscTabs", size, false, ImGuiWindowFlags_AlwaysUseWindowPadding);
	{
		ImGui::Spacing();

		ImGui::Indent(vars->widgetOffset);
		ImGui::PushStyleColor(ImGuiCol_Text, Colors::White.imGui());
		ImGui::PushFont(titleFont);
		ImGui::Text("Hacks");
		ImGui::PopFont();
		ImGui::PopStyleColor();
		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

		// For gears
		constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar;
		static auto screenPos = ImVec2{ 0.f, 0.f };

		hmHelper->m_pGui->ToggleButton("Anti shield", &hmHelper->m_pConfig->antiShielded.enabled);
		hmHelper->m_pGui->ToggleButton("No camera shake", &hmHelper->m_pConfig->noCameraShake.enabled);
		hmHelper->m_pGui->ToggleButton("No spread", &hmHelper->m_pConfig->noSpread.enabled);
		hmHelper->m_pGui->ToggleButton("Inf ammo", &hmHelper->m_pConfig->infAmmo.enabled);
		hmHelper->m_pGui->ToggleButton("Rapid fire", &hmHelper->m_pConfig->rapidFire.enabled, nullptr, nullptr, &hmHelper->m_pConfig->rapidFire.gear, &screenPos);
		if (hmHelper->m_pConfig->rapidFire.gear)
		{
			// Render the gear menu
			Colors::tabsBgColor.a = 255.f;
			const auto menuSize = ImVec2(350.f, 0.f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0)); // Remove free space around the window
			ImGui::PushStyleColor(ImGuiCol_WindowBg, Colors::tabsBgColor.imGui());
			ImGui::SetNextWindowPos(ImVec2(screenPos.x, screenPos.y), ImGuiCond_Appearing);
			ImGui::SetNextWindowSize(menuSize);
			ImGui::Begin("#rapidFireGear", nullptr, flags);

			ImGui::Spacing();
			ImGui::Indent(vars->childWindowOffset);

			ImGui::PushStyleColor(ImGuiCol_Text, Colors::White.imGui());
			ImGui::PushFont(titleFont);
			ImGui::Text("Rapid Fire");
			ImGui::PopFont();
			ImGui::PopStyleColor();
			ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.4f);
			ImGui::SliderFloat("Rifle fire delay", &hmHelper->m_pConfig->rapidFire.useDelayRifle, 0.05f, 0.1f);
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.4f);
			ImGui::SliderFloat("Weapons fire delay", &hmHelper->m_pConfig->rapidFire.useDelayOther, 0.3f, 5.f);

			ImGui::Unindent();
			ImGui::Spacing();
			ImGui::End();
			ImGui::PopStyleVar();
			ImGui::PopStyleVar();
			ImGui::PopStyleColor();
		}

		hmHelper->m_pGui->ToggleButton("Custom Damage", &hmHelper->m_pConfig->customDamage.enabled, nullptr, nullptr, &hmHelper->m_pConfig->customDamage.gear, &screenPos);
		if (hmHelper->m_pConfig->customDamage.gear)
		{
			// Render the gear menu
			Colors::tabsBgColor.a = 255.f;
			const auto menuSize = ImVec2(350.f, 0.f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0)); // Remove free space around the window
			ImGui::PushStyleColor(ImGuiCol_WindowBg, Colors::tabsBgColor.imGui());
			ImGui::SetNextWindowPos(ImVec2(screenPos.x, screenPos.y), ImGuiCond_Appearing);
			ImGui::SetNextWindowSize(menuSize);
			ImGui::Begin("#customDamage", nullptr, flags);

			ImGui::Spacing();
			ImGui::Indent(vars->childWindowOffset);

			ImGui::PushStyleColor(ImGuiCol_Text, Colors::White.imGui());
			ImGui::PushFont(titleFont);
			ImGui::Text("Custom Damage");
			ImGui::PopFont();
			ImGui::PopStyleColor();
			ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

			static const char* damageType[4] = { "Rifle", "Shotgun", "Sniper", "Revolver" };
			hmHelper->m_pGui->ToggleButton("Use global", &hmHelper->m_pConfig->customDamage.useGlobal);
			if (hmHelper->m_pConfig->customDamage.useGlobal)
			{
				static int selectedDamageType = hmHelper->GetIdxFromDmgTypeSelection(hmHelper->m_pConfig->customDamage.global);
				WCombo("Global", &selectedDamageType, damageType, IM_ARRAYSIZE(damageType));
				hmHelper->m_pConfig->customDamage.global = hmHelper->GetDmgTypeFromIdx(selectedDamageType);
			}
			else
			{
				static int rifleDamageType = hmHelper->GetIdxFromDmgTypeSelection(hmHelper->m_pConfig->customDamage.rifle);
				WCombo("Rifle", &rifleDamageType, damageType, IM_ARRAYSIZE(damageType));
				hmHelper->m_pConfig->customDamage.rifle = hmHelper->GetDmgTypeFromIdx(rifleDamageType);

				static int shotgunDamageType = hmHelper->GetIdxFromDmgTypeSelection(hmHelper->m_pConfig->customDamage.shotgun);
				WCombo("Shotgun", &shotgunDamageType, damageType, IM_ARRAYSIZE(damageType));
				hmHelper->m_pConfig->customDamage.shotgun = hmHelper->GetDmgTypeFromIdx(shotgunDamageType);

				static int sniperDamageType = hmHelper->GetIdxFromDmgTypeSelection(hmHelper->m_pConfig->customDamage.sniper);
				WCombo("Sniper", &sniperDamageType, damageType, IM_ARRAYSIZE(damageType));
				hmHelper->m_pConfig->customDamage.sniper = hmHelper->GetDmgTypeFromIdx(sniperDamageType);

				static int revolverDamageType = hmHelper->GetIdxFromDmgTypeSelection(hmHelper->m_pConfig->customDamage.revolver);
				WCombo("Revolver", &revolverDamageType, damageType, IM_ARRAYSIZE(damageType));
				hmHelper->m_pConfig->customDamage.revolver = hmHelper->GetDmgTypeFromIdx(revolverDamageType);
			}

			ImGui::Unindent();
			ImGui::Spacing();
			ImGui::End();
			ImGui::PopStyleVar();
			ImGui::PopStyleVar();
			ImGui::PopStyleColor();
		}

		hmHelper->m_pGui->ToggleButton("Fly hack", &hmHelper->m_pConfig->flyHack.enabled);
		hmHelper->m_pGui->ToggleButton("Fast melee", &hmHelper->m_pConfig->fastMeele.enabled);

		ImGui::Unindent();
		ImGui::Spacing();
		sizeDiffs[0] = ImGui::GetCursorPosY();
	}

	ImGui::EndChild();
	ImGui::PopStyleColor();
	ImGui::PopStyleVar();
	ImGui::Unindent();
}

void HMGui::RenderEspTab()
{
	ImGui::Indent(vars->childWindowOffset);
	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 10.f);
	ImGui::PushStyleColor(ImGuiCol_ChildBg, Colors::tabsBgColor.imGui());
	static float sizeDiffs[4] = { 0.f, 0.f, 0.f, 0.f };
	ImVec2 size = CalculateWindowSize(vars->childWindowOffset);
	size.y = sizeDiffs[0];
	ImGui::BeginChild("##espTabs", size, false, ImGuiWindowFlags_AlwaysUseWindowPadding);
	{
		ImGui::Spacing();

		ImGui::Indent(vars->widgetOffset);
		ImGui::PushStyleColor(ImGuiCol_Text, Colors::White.imGui());
		ImGui::PushFont(titleFont);
		ImGui::Text("Players");
		ImGui::PopFont();
		ImGui::PopStyleColor();
		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

		// For gears
		constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar;
		static auto screenPos = ImVec2{ 0.f, 0.f };

		hmHelper->m_pGui->ToggleButton("Enabled", &hmHelper->m_pConfig->playerEsp.enabled);
		hmHelper->m_pGui->ToggleButton("Nameplates", &hmHelper->m_pConfig->playerEsp.nameplates);
		hmHelper->m_pGui->ToggleButton("Box", &hmHelper->m_pConfig->playerEsp.box);


		ImGui::Unindent();
		ImGui::Spacing();
		sizeDiffs[0] = ImGui::GetCursorPosY();
	}

	ImGui::EndChild();
	ImGui::PopStyleColor();
	ImGui::PopStyleVar();
	ImGui::Unindent();
}
