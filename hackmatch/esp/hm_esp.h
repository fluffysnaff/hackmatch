#pragma once

#include "../il2cpp/il2cpp.h"

class HMEsp
{
public:
	void RenderEsp();

	// Helpers
	float GetPlayerHeight(ImVec2 head, ImVec2 feet)
	{
		const float temp1 = powf(head.x - feet.x, 2.f);
		const float temp2 = powf(head.y - feet.y, 2.f);
		const float height = sqrtf(temp1 + temp2);
		return height;
	}

private:
	void RenderNameplates();
	void RenderBox();
};
