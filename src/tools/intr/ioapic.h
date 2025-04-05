// ------------------------------------------------------------------------------------------------
// intr/ioapic.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "src/tools/stdlib/types.h"

extern u8 *g_ioApicAddr;

void IoApicInit();
void IoApicSetEntry(u8 *base, u8 index, u64 data);
