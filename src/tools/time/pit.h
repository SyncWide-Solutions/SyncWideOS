// ------------------------------------------------------------------------------------------------
// time/pit.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "src/tools/stdlib/types.h"

extern volatile u32 g_pitTicks;

void PitInit();
void PitWait(uint ms);
