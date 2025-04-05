// ------------------------------------------------------------------------------------------------
// gfx/gfxdisplay.c
// ------------------------------------------------------------------------------------------------

#include "src/tools/gfx/gfxdisplay.h"
#include "src/tools/gfx/reg.h"
#include "src/tools/cpu/io.h"
#include "src/tools/time/pit.h"
#include "src/tools/networking/rlog.h"

// ------------------------------------------------------------------------------------------------
void GfxInitDisplay(GfxDisplay *display)
{
    // MWDD FIX: To Do
    display->dummy = 0;
}

// ------------------------------------------------------------------------------------------------
void GfxDisableVga(GfxPci *pci)
{
    IoWrite8(SR_INDEX, SEQ_CLOCKING);
    IoWrite8(SR_DATA, IoRead8(SR_DATA) | SCREEN_OFF);
    PitWait(100);
    GfxWrite32(pci, VGA_CONTROL, VGA_DISABLE);

    RlogPrint("VGA Plane disabled\n");
}
