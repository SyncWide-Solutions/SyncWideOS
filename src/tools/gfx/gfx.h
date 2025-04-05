// ------------------------------------------------------------------------------------------------
// gfx/gfx.h
// ------------------------------------------------------------------------------------------------

#pragma once

#include "src/tools/pci/driver.h"

void GfxInit(uint id, PciDeviceInfo *info);
void GfxStart();
void GfxPoll();
