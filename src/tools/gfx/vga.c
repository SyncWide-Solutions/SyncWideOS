// ------------------------------------------------------------------------------------------------
// gfx/vga.c
// ------------------------------------------------------------------------------------------------

#include "src/tools/gfx/vga.h"
#include "src/tools/cpu/io.h"
#include "src/tools/mem/lowmem.h"

// ------------------------------------------------------------------------------------------------
void VgaTextInit()
{
    VgaTextClear();
    VgaTextSetCursor(0);
}

// ------------------------------------------------------------------------------------------------
void VgaTextClear()
{
    volatile u64 *p = (volatile u64 *)VGA_TEXT_BASE;
    volatile u64 *end = p + 500;

    for (; p != end; ++p)
    {
        *p = 0x1f201f201f201f20;
    }
}

// ------------------------------------------------------------------------------------------------
void VgaTextSetCursor(uint offset)
{
    IoWrite8(0x3d4, 0x0e);
    IoWrite8(0x3d5, offset >> 8);
    IoWrite8(0x3d4, 0x0f);
    IoWrite8(0x3d5, offset);
}
