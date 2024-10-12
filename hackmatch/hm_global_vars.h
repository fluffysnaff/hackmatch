#pragma once

#include <memory>

class HMVariables
{
public:
#define MACRO_CONFIG_INT(Name, value) int Name = value;
#define MACRO_CONFIG_FLOAT(Name, value) float Name = value;
#define MACRO_CONFIG_STR(Name, value) const char* Name = value;
#include "hm_variables.h"
#undef MACRO_CONFIG_INT
#undef MACRO_CONFIG_FLOAT
#undef MACRO_CONFIG_STR
};

extern std::unique_ptr<HMVariables> vars;
