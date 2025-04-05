// ------------------------------------------------------------------------------------------------
// time/rtc.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "src/tools/time/time.h"

// ------------------------------------------------------------------------------------------------
void RtcGetTime(DateTime *dt);
void RtcSetTime(const DateTime *dt);
