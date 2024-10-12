#pragma once

#include <vector>

struct HookedFunction
{
	uintptr_t orig;
	void* target;
	void* callback;
};

class HMHooks
{
public:
	void Init();
	bool HookFunction(void* funPtr, void* callbackFun);

	std::vector<HookedFunction> hooks;

};
