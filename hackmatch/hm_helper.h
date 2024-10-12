#pragma once

#include <memory>

#include "il2cpp/hm_il2cpp_helper.h"
#include "hm_hacks.h"
#include "hm_config.h"
#include "gui/hm_gui.h"
#include "hooks/hm_hooks.h"
#include "sdk/hm_sdk.h"
#include "esp/hm_esp.h"

#define ASSERT(x) { MessageBoxA(NULL, NULL, x, NULL); exit(0); }

class HMHelper
{
public:
	HMHelper();

	std::unique_ptr<IL2CppHelper> m_pIl2cpp;
	std::unique_ptr<HMHacks>	  m_pHacks;
	std::unique_ptr<HMConfig>	  m_pConfig;
	std::unique_ptr<HMGui>		  m_pGui;
	std::unique_ptr<HMHooks>	  m_pHooks;
	std::unique_ptr<HMSdk>		  m_pSdk;
	std::unique_ptr<HMEsp>		  m_pEsp;

	int GetIdxFromDmgTypeSelection(DamageLobyDataEnum selection);
	DamageLobyDataEnum GetDmgTypeFromIdx(int selection);
};

extern std::unique_ptr<HMHelper> hmHelper;
