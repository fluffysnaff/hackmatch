#include <filesystem>
#include <string>

#include "../data/fonts/roboto.h"
#include "../data/fonts/fa_data.h"
#include "../data/fonts/fa.h"

#include "hm_gui.h"


void HMGui::SetupImGuiFonts()
{
	// Setup Dear ImGui font
	const ImGuiIO& io = ImGui::GetIO();
	const auto ranges = GetFontGlyphRanges();
	ImFontConfig defCfg;
	defCfg.OversampleH = defCfg.OversampleV = 2;
	defCfg.RasterizerMultiply = 1.4f;
	defCfg.FontDataOwnedByAtlas = false;

	io.Fonts->AddFontFromMemoryTTF(const_cast<std::uint8_t*>(robotoData),
		sizeof(robotoData),
		20.f,
		&defCfg,
		ranges);



	// Setup icon font - Needs to be 2nd in row
	ImFontConfig iconConfig;
	iconConfig.MergeMode = true;
	iconConfig.PixelSnapH = true;
	ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(font_awesome_data,
		font_awesome_size,
		18.f,
		&iconConfig,
		ranges);

	// --- Setup fonts ---
	// Check if calibri font exists, otherwise use fallback
	std::string calibriPath = "C:\\Windows\\Fonts\\Calibri.ttf";
	std::filesystem::path f{ calibriPath.c_str() };
	if (std::filesystem::exists(f))
		espFont = AddFont(17.f, calibriPath.c_str());
	else
	{
		// We can always be sure this is added since it's in memory
		espFont = io.Fonts->AddFontFromMemoryTTF(
			const_cast<std::uint8_t*>(robotoData), sizeof(robotoData), 17.f, &defCfg, ranges);
	}

	// Merge simplified chineese font to espFont
	// Check if it exists
	// std::string notosansPath = fontPath + "NotoSansSC-SemiBold.ttf";
	// std::filesystem::path f1{ notosansPath.c_str() };
	// if (std::filesystem::exists(f1))
	// {
	// 	ImFontConfig mergeFontConfig;
	// 	mergeFontConfig.OversampleH = mergeFontConfig.OversampleV = 2;
	// 	mergeFontConfig.RasterizerMultiply = 1.4f;
	// 	mergeFontConfig.MergeMode = true;
	// 	mergeFontConfig.PixelSnapH = true;
	// 	io.Fonts->AddFontFromFileTTF(notosansPath.c_str(), 14.f, &mergeFontConfig, ranges);
	// }
	subFont = io.Fonts->AddFontFromMemoryTTF(const_cast<std::uint8_t*>(robotoData),
		sizeof(robotoData),
		15.f,
		&defCfg,
		ranges);
	titleFont = io.Fonts->AddFontFromMemoryTTF(const_cast<std::uint8_t*>(robotoData),
		sizeof(robotoData),
		23.f,
		&defCfg,
		ranges);

	io.Fonts->Build();

}

void HMGui::SetupImGuiStyle()
{
	ImGuiStyle& style = ImGui::GetStyle();

	// Set style
	style.WindowRounding = 3.f;

	style.ChildRounding = 3.f;
	style.ChildBorderSize = 1.f;

	style.FrameRounding = 3.f;
	style.FrameBorderSize = 1.f;

	style.GrabRounding = 64.f;
	style.GrabMinSize = 10.f;
	style.WindowMenuButtonPosition = ImGuiDir_Right;

	// Scale using DPI
	// style.ScaleAllSizes(vars->dpi / USER_DEFAULT_SCREEN_DPI);

	// Colors
	style.Colors[ImGuiCol_Border] = Colors::UltramarineBlueLow.imVec4();
	style.Colors[ImGuiCol_CheckMark] = Colors::UltramarineBlue.imVec4();
	style.Colors[ImGuiCol_Text] = Colors::mainFontColor.imVec4();

	style.Colors[ImGuiCol_TitleBg] = Colors::UltramarineBlue.imVec4();
	style.Colors[ImGuiCol_TitleBgActive] = Colors::UltramarineBlue.imVec4();

	style.Colors[ImGuiCol_WindowBg] = Colors::windowBgColor.imVec4();
	style.Colors[ImGuiCol_ChildBg] = Colors::childBgColor.imVec4();

	style.Colors[ImGuiCol_Button] = Colors::buttonColor.imVec4();
	style.Colors[ImGuiCol_ButtonHovered] = Colors::buttonHoveredColor.imVec4();
	style.Colors[ImGuiCol_ButtonActive] = Colors::buttonActiveColor.imVec4();

	style.Colors[ImGuiCol_FrameBg] = style.Colors[ImGuiCol_WindowBg]; // Invisible frame bg
	style.Colors[ImGuiCol_FrameBgHovered] = Colors::frameColor.imVec4();
	style.Colors[ImGuiCol_FrameBgActive] = Colors::frameColor.imVec4();

	style.Colors[ImGuiCol_Header] = Colors::headerColor.imVec4();
	style.Colors[ImGuiCol_HeaderActive] = Colors::headerColor.imVec4();
	style.Colors[ImGuiCol_HeaderHovered] = Colors::headerColor.imVec4();

	style.Colors[ImGuiCol_Separator] = Colors::UltramarineBlue.imVec4();
	style.Colors[ImGuiCol_SeparatorHovered] = Colors::UltramarineBlue.imVec4();
	style.Colors[ImGuiCol_SeparatorActive] = Colors::UltramarineBlue.imVec4();
}


// Base
ImFont* HMGui::AddFont(float fontSize, const char* fontPath, ImFontConfig* fontCfg)
{
	ImGuiIO& io = ImGui::GetIO();
	(void)io;

	ImFontConfig cfg;
	cfg.OversampleH = cfg.OversampleV = 2;
	cfg.RasterizerMultiply = 1.4f;
	cfg.FontDataOwnedByAtlas = false;

	if (fontCfg != nullptr)
		cfg = *fontCfg;

	const auto ranges = GetFontGlyphRanges();
	ImFont* font = io.Fonts->AddFontFromFileTTF(fontPath, fontSize, &cfg, ranges);
	return font;
}

ImWchar* HMGui::GetFontGlyphRanges() noexcept
{
	static ImVector<ImWchar> ranges;
	if (ranges.empty())
	{
		ImFontGlyphRangesBuilder builder;
		constexpr ImWchar baseRanges[]{ 0x0100, 0x024F, // Latin Extended-A + Latin Extended-B
									   0x0300, 0x03FF, // Combining Diacritical Marks + Greek/Coptic
									   0x0600, 0x06FF, // Arabic
									   0x0E00, 0x0E7F, // Thai
									   ICON_MIN_FA, ICON_MAX_FA,
									   0 };
		builder.AddRanges(baseRanges);
		builder.AddRanges(ImGui::GetIO().Fonts->GetGlyphRangesCyrillic());
		builder.AddRanges(ImGui::GetIO().Fonts->GetGlyphRangesChineseSimplifiedCommon());
		builder.AddText("\u9F8D\u738B\u2122\u00E4\u00C4");
		builder.BuildRanges(&ranges);
	}
	return ranges.Data;
}

ImVec2 HMGui::CalculateWindowSize(float childWindowOffset, ImVec2 windows)
{
	ImVec2 size = ImGui::GetContentRegionAvail();
	const float windowSizeX = childWindowOffset * windows.x;
	const float windowSizeY = 1 / windows.y;
	size.x = (size.x - windowSizeX) * ceil(windowSizeY * 100.f) / 100.f; // Rounds to 2 decimal places
	size.y = (size.y - windowSizeX) * ceil(windowSizeY * 100.f) / 100.f; // Rounds to 2 decimal places
	return size;
}
