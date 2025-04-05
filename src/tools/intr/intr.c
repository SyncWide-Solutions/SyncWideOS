// ------------------------------------------------------------------------------------------------
// intr/intr.c
// ------------------------------------------------------------------------------------------------

#include "src/tools/intr/intr.h"
#include "src/tools/intr/idt.h"
#include "src/tools/intr/ioapic.h"
#include "src/tools/intr/local_apic.h"
#include "src/tools/intr/pic.h"
#include "src/tools/acpi/acpi.h"
#include "src/tools/time/pit.h"

// ------------------------------------------------------------------------------------------------
extern void pit_interrupt();
extern void spurious_interrupt();

// ------------------------------------------------------------------------------------------------
void IntrInit()
{
    // Build Interrupt Table
    IdtInit();
    IdtSetHandler(INT_TIMER, INTERRUPT_GATE, pit_interrupt);
    IdtSetHandler(INT_SPURIOUS, INTERRUPT_GATE, spurious_interrupt);

    // Initialize subsystems
    PicInit();
    LocalApicInit();
    IoApicInit();
    PitInit();

    // Enable IO APIC entries
    IoApicSetEntry(g_ioApicAddr, AcpiRemapIrq(IRQ_TIMER), INT_TIMER);

    // Enable all interrupts
    __asm__ volatile("sti");
}
