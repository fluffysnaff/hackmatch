#pragma once

#include <mutex>
#include <vector>
#include "../il2cpp/il2cpp.h"

class HMSdk
{
public:
	PlayerController_o* GetLocalPlayer();

	// All players in game
	void UpdatePlayer(PlayerController_o* thisptr);
	std::vector<PlayerController_o*> players {};
	std::mutex playersMutex {};
};
