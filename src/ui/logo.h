#pragma once

#include <imgui.h>

struct ID3D11Device;

namespace hackmatch::ui {
bool init_logo(ID3D11Device* device);
void shutdown_logo();
void draw_logo(ImDrawList* draw, ImVec2 position, float size, ImU32 tint);
}
