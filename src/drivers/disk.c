#include "../include/disk.h"
#include "../include/io.h"
#include "../include/string.h"
#include <stddef.h>

// External function declarations
extern void terminal_writestring(const char* data);

// Global variables
ide_channel_t channels[2];
ide_device_t ide_devices[4];
static uint8_t ide_buf[2048] = {0};
static uint8_t ide_irq_invoked = 0;

// Forward declarations
uint8_t ide_read(uint8_t channel, uint8_t reg);
void ide_write(uint8_t channel, uint8_t reg, uint8_t data);
void ide_read_buffer(uint8_t channel, uint8_t reg, uint32_t buffer, uint32_t quads);

// Initialize IDE controller
void ide_initialize(uint32_t bar0, uint32_t bar1, uint32_t bar2, uint32_t bar3, uint32_t bar4) {
    int k, count = 0;

    // 1- Detect I/O Ports which interface IDE Controller:
    channels[ATA_PRIMARY].base = (bar0 & 0xFFFFFFFC) + 0x1F0 * (!bar0);
    channels[ATA_PRIMARY].ctrl = (bar1 & 0xFFFFFFFC) + 0x3F6 * (!bar1);
    channels[ATA_SECONDARY].base = (bar2 & 0xFFFFFFFC) + 0x170 * (!bar2);
    channels[ATA_SECONDARY].ctrl = (bar3 & 0xFFFFFFFC) + 0x376 * (!bar3);
    channels[ATA_PRIMARY].bmide = (bar4 & 0xFFFFFFFC) + 0; // Bus Master IDE
    channels[ATA_SECONDARY].bmide = (bar4 & 0xFFFFFFFC) + 8; // Bus Master IDE

    // 2- Disable IRQs:
    ide_write(ATA_PRIMARY, ATA_REG_CONTROL, 2);
    ide_write(ATA_SECONDARY, ATA_REG_CONTROL, 2);

    // 3- Detect ATA-ATAPI Devices:
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2; j++) {
            uint8_t err = 0, type = IDE_ATA, status;
            ide_devices[count].reserved = 0; // Assuming that no drive here.

            // (I) Select Drive:
            ide_write(i, ATA_REG_HDDEVSEL, 0xA0 | (j << 4)); // Select Drive.
            for (k = 0; k < 1000; k++); // Wait 1ms for drive select to work.

            // (II) Send ATA Identify Command:
            ide_write(i, ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
            for (k = 0; k < 1000; k++); // This function should be implemented in your OS.

            // (III) Polling:
            if (ide_read(i, ATA_REG_STATUS) == 0) continue; // If Status = 0, No Device.

            while(1) {
                status = ide_read(i, ATA_REG_STATUS);
                if ((status & ATA_SR_ERR)) {err = 1; break;} // If Err, Device is not ATA.
                if (!(status & ATA_SR_BSY) && (status & ATA_SR_DRQ)) break; // Everything is right.
            }

            // (IV) Probe for ATAPI Devices:
            if (err != 0) {
                uint8_t cl = ide_read(i, ATA_REG_LBA1);
                uint8_t ch = ide_read(i, ATA_REG_LBA2);

                if (cl == 0x14 && ch == 0xEB)
                    type = IDE_ATAPI;
                else if (cl == 0x69 && ch == 0x96)
                    type = IDE_ATAPI;
                else
                    continue; // Unknown Type (may not be a device).

                ide_write(i, ATA_REG_COMMAND, ATA_CMD_IDENTIFY_PACKET);
                for (k = 0; k < 1000; k++);
            }

            // (V) Read Identification Space of the Device:
            ide_read_buffer(i, ATA_REG_DATA, (uint32_t) ide_buf, 128);

            // (VI) Read Device Parameters:
            ide_devices[count].reserved     = 1;
            ide_devices[count].type         = type;
            ide_devices[count].channel      = i;
            ide_devices[count].drive        = j;
            ide_devices[count].signature    = *((uint16_t *)(ide_buf + ATA_IDENT_DEVICETYPE));
            ide_devices[count].capabilities = *((uint16_t *)(ide_buf + ATA_IDENT_CAPABILITIES));
            ide_devices[count].command_sets = *((uint32_t *)(ide_buf + ATA_IDENT_COMMANDSETS));

            // (VII) Get Size:
            if (ide_devices[count].command_sets & (1 << 26))
                // Device uses 48-Bit Addressing:
                ide_devices[count].size = *((uint32_t *)(ide_buf + ATA_IDENT_MAX_LBA_EXT));
            else
                // Device uses CHS or 28-bit Addressing:
                ide_devices[count].size = *((uint32_t *)(ide_buf + ATA_IDENT_MAX_LBA));

            // (VIII) String indicates model of device (like Western Digital HDD and SONY DVD-RW...):
            for(k = 0; k < 40; k += 2) {
                ide_devices[count].model[k] = ide_buf[ATA_IDENT_MODEL + k + 1];
                ide_devices[count].model[k + 1] = ide_buf[ATA_IDENT_MODEL + k];
            }
            ide_devices[count].model[40] = 0; // Terminate String.

            count++;
        }
    }

    // 4- Print Summary:
    for (int i = 0; i < 4; i++) {
        if (ide_devices[i].reserved == 1) {
            terminal_writestring(" Found ");
            terminal_writestring((ide_devices[i].type == IDE_ATA) ? "ATA" : "ATAPI");
            terminal_writestring(" Drive ");
            char size_str[32];
            itoa(ide_devices[i].size / 1024 / 2, size_str, 10); // Convert to MB
            terminal_writestring(size_str);
            terminal_writestring("MB - ");
            terminal_writestring((char*)ide_devices[i].model);
            terminal_writestring("\n");
        }
    }
}

// Read from IDE register
uint8_t ide_read(uint8_t channel, uint8_t reg) {
    uint8_t result;
    if (reg > 0x07 && reg < 0x0C)
        ide_write(channel, ATA_REG_CONTROL, 0x80 | channels[channel].nien);
    if (reg < 0x08)
        result = inb(channels[channel].base + reg - 0x00);
    else if (reg < 0x0C)
        result = inb(channels[channel].base + reg - 0x06);
    else if (reg < 0x0E)
        result = inb(channels[channel].ctrl + reg - 0x0A);
    else if (reg < 0x16)
        result = inb(channels[channel].bmide + reg - 0x0E);
    if (reg > 0x07 && reg < 0x0C)
        ide_write(channel, ATA_REG_CONTROL, channels[channel].nien);
    return result;
}

// Write to IDE register
void ide_write(uint8_t channel, uint8_t reg, uint8_t data) {
    if (reg > 0x07 && reg < 0x0C)
        ide_write(channel, ATA_REG_CONTROL, 0x80 | channels[channel].nien);
    if (reg < 0x08)
        outb(channels[channel].base + reg - 0x00, data);
    else if (reg < 0x0C)
        outb(channels[channel].base + reg - 0x06, data);
    else if (reg < 0x0E)
        outb(channels[channel].ctrl + reg - 0x0A, data);
    else if (reg < 0x16)
        outb(channels[channel].bmide + reg - 0x0E, data);
    if (reg > 0x07 && reg < 0x0C)
        ide_write(channel, ATA_REG_CONTROL, channels[channel].nien);
}

// Read buffer from IDE
void ide_read_buffer(uint8_t channel, uint8_t reg, uint32_t buffer, uint32_t quads) {
    if (reg > 0x07 && reg < 0x0C)
        ide_write(channel, ATA_REG_CONTROL, 0x80 | channels[channel].nien);
    
    if (reg < 0x08)
        insw(channels[channel].base + reg - 0x00, (uint16_t*)buffer, quads * 2);
    else if (reg < 0x0C)
        insw(channels[channel].base + reg - 0x06, (uint16_t*)buffer, quads * 2);
    else if (reg < 0x0E)
        insw(channels[channel].ctrl + reg - 0x0A, (uint16_t*)buffer, quads * 2);
    else if (reg < 0x16)
        insw(channels[channel].bmide + reg - 0x0E, (uint16_t*)buffer, quads * 2);
    
    if (reg > 0x07 && reg < 0x0C)
        ide_write(channel, ATA_REG_CONTROL, channels[channel].nien);
}

// Polling function
uint8_t ide_polling(uint8_t channel, uint32_t advanced_check) {
    // (I) Delay 400 nanosecond for BSY to be set:
    for(int i = 0; i < 4; i++)
        ide_read(channel, ATA_REG_ALTSTATUS); // Reading the Alternate Status port wastes 100ns; loop four times.

    // (II) Wait for BSY to be cleared:
    while (ide_read(channel, ATA_REG_STATUS) & ATA_SR_BSY)
        ; // Wait for BSY to be zero.

    if (advanced_check) {
        uint8_t state = ide_read(channel, ATA_REG_STATUS); // Read Status Register.

        // (III) Check For Errors:
        if (state & ATA_SR_ERR)
            return 2; // Error.

        // (IV) Check If Device fault:
        if (state & ATA_SR_DF)
            return 1; // Device Fault.

        // (V) Check DRQ:
        // BSY = 0; DF = 0; ERR = 0 so we should check for DRQ now.
        if ((state & ATA_SR_DRQ) == 0)
            return 3; // DRQ should be set

    }

    return 0; // No Error.
}

// Print error function
uint8_t ide_print_error(uint32_t drive, uint8_t err) {
    if (err == 0)
        return err;

    terminal_writestring("IDE:");
    if (err == 1) {
        terminal_writestring("- Device Fault\n     ");
    } else if (err == 2) {
        uint8_t st = ide_read(ide_devices[drive].channel, ATA_REG_ERROR);
        if (st & ATA_ER_AMNF)   terminal_writestring("- No Address Mark Found\n     ");
        if (st & ATA_ER_TK0NF)  terminal_writestring("- No Media or Media Error\n     ");
        if (st & ATA_ER_ABRT)   terminal_writestring("- Command Aborted\n     ");
        if (st & ATA_ER_MCR)    terminal_writestring("- No Media or Media Error\n     ");
        if (st & ATA_ER_IDNF)   terminal_writestring("- ID mark not Found\n     ");
        if (st & ATA_ER_MC)     terminal_writestring("- No Media or Media Error\n     ");
        if (st & ATA_ER_UNC)    terminal_writestring("- Uncorrectable Data Error\n     ");
        if (st & ATA_ER_BBK)    terminal_writestring("- Bad Sectors\n     ");
    } else if (err == 3) {
        terminal_writestring("- Reads Nothing\n     ");
    } else if (err == 4) {
        terminal_writestring("- Write Protected\n     ");
    }
    terminal_writestring("- [");
    terminal_writestring((ide_devices[drive].type == IDE_ATA) ? "ATA" : "ATAPI");
    terminal_writestring("] ");
    terminal_writestring((char*)ide_devices[drive].model);
    terminal_writestring("\n");

    return err;
}

// Initialize disk subsystem
void disk_init(void) {
    // Initialize IDE controller with default I/O ports
    ide_initialize(0x1F0, 0x3F6, 0x170, 0x376, 0x000);
}

// Detect drives
void disk_detect_drives(void) {
    // This is already done in ide_initialize
    terminal_writestring("Disk detection completed.\n");
}

// Get drive count
int disk_get_drive_count(void) {
    int count = 0;
    for (int i = 0; i < 4; i++) {
        if (ide_devices[i].reserved == 1) {
            count++;
        }
    }
    return count;
}

// Get drive info
ide_device_t* disk_get_drive_info(int drive_index) {
    if (drive_index < 0 || drive_index >= 4) {
        return NULL;
    }
    
    if (ide_devices[drive_index].reserved == 0) {
        return NULL;
    }
    
    return &ide_devices[drive_index];
}

// Print detailed drive information
void disk_print_drive_info(int drive_index) {
    if (drive_index < 0 || drive_index >= 4) {
        terminal_writestring("Invalid drive index\n");
        return;
    }
    
    if (ide_devices[drive_index].reserved == 0) {
        terminal_writestring("No drive at index ");
        char buffer[16];
        itoa(drive_index, buffer, 10);
        terminal_writestring(buffer);
        terminal_writestring("\n");
        return;
    }
    
    ide_device_t* drive = &ide_devices[drive_index];
    char buffer[32];
    
    terminal_writestring("  Drive ");
    itoa(drive_index, buffer, 10);
    terminal_writestring(buffer);
    terminal_writestring(":\n");
    
    terminal_writestring("    Type: ");
    terminal_writestring((drive->type == IDE_ATA) ? "ATA (Hard Disk)" : "ATAPI (CD/DVD)");
    terminal_writestring("\n");
    
    terminal_writestring("    Channel: ");
    terminal_writestring((drive->channel == ATA_PRIMARY) ? "Primary" : "Secondary");
    terminal_writestring("\n");
    
    terminal_writestring("    Position: ");
    terminal_writestring((drive->drive == 0) ? "Master" : "Slave");
    terminal_writestring("\n");
    
    terminal_writestring("    Model: ");
    // Trim spaces from model string
    char* model_start = (char*)drive->model;
    while (*model_start == ' ') model_start++;
    char* model_end = model_start + strlen(model_start) - 1;
    while (model_end > model_start && *model_end == ' ') {
        *model_end = '\0';
        model_end--;
    }
    terminal_writestring(model_start);
    terminal_writestring("\n");
    
    if (drive->type == IDE_ATA) {
        terminal_writestring("    Size: ");
        uint32_t size_mb = drive->size / 2048; // Convert sectors to MB (assuming 512 bytes per sector)
        itoa(size_mb, buffer, 10);
        terminal_writestring(buffer);
        terminal_writestring(" MB (");
        
        if (size_mb >= 1024) {
            uint32_t size_gb = size_mb / 1024;
            itoa(size_gb, buffer, 10);
            terminal_writestring(buffer);
            terminal_writestring(" GB");
        } else {
            terminal_writestring(buffer);
            terminal_writestring(" MB");
        }
        terminal_writestring(")\n");
        
        terminal_writestring("    Sectors: ");
        itoa(drive->size, buffer, 10);
        terminal_writestring(buffer);
        terminal_writestring("\n");
        
        terminal_writestring("    Addressing: ");
        if (drive->command_sets & (1 << 26)) {
            terminal_writestring("48-bit LBA");
        } else {
            terminal_writestring("28-bit LBA/CHS");
        }
        terminal_writestring("\n");
    }
    
    terminal_writestring("    Signature: 0x");
    itoa(drive->signature, buffer, 16);
    terminal_writestring(buffer);
    terminal_writestring("\n");
    
    terminal_writestring("    Capabilities: 0x");
    itoa(drive->capabilities, buffer, 16);
    terminal_writestring(buffer);
    terminal_writestring("\n");
    
    terminal_writestring("    Command Sets: 0x");
    itoa(drive->command_sets, buffer, 16);
    terminal_writestring(buffer);
    terminal_writestring("\n");
    
    // Print some capability flags
    terminal_writestring("    Features: ");
    if (drive->capabilities & (1 << 9)) terminal_writestring("LBA ");
    if (drive->capabilities & (1 << 8)) terminal_writestring("DMA ");
    if (drive->command_sets & (1 << 26)) terminal_writestring("LBA48 ");
    if (drive->command_sets & (1 << 10)) terminal_writestring("HPA ");
    if (drive->command_sets & (1 << 5)) terminal_writestring("PUIS ");
    if (drive->command_sets & (1 << 3)) terminal_writestring("APM ");
    if (drive->command_sets & (1 << 2)) terminal_writestring("CFA ");
    if (drive->command_sets & (1 << 1)) terminal_writestring("TCQ ");
    terminal_writestring("\n");
}

// Read sectors (basic implementation)
uint8_t ide_read_sectors(uint8_t drive, uint8_t numsects, uint32_t lba, uint16_t es, uint32_t edi) {
    uint8_t lba_mode, dma, cmd;
    uint8_t lba_io[6];
    uint32_t channel = ide_devices[drive].channel;
    uint32_t slavebit = ide_devices[drive].drive;
    uint32_t bus = channels[channel].base;
    uint32_t words = 256;
    uint16_t cyl, i;
    uint8_t head, sect, err;

    // (I) Select one from LBA28, LBA48 or CHS;
    if (lba >= 0x10000000) {
        // LBA48:
        lba_mode = 2;
        lba_io[0] = (lba & 0x000000FF) >> 0;
        lba_io[1] = (lba & 0x0000FF00) >> 8;
        lba_io[2] = (lba & 0x00FF0000) >> 16;
        lba_io[3] = (lba & 0xFF000000) >> 24;
        lba_io[4] = 0;
        lba_io[5] = 0;
        head = 0;
    } else if (ide_devices[drive].capabilities & 0x200) {
        // LBA28:
        lba_mode = 1;
        lba_io[0] = (lba & 0x00000FF) >> 0;
        lba_io[1] = (lba & 0x000FF00) >> 8;
        lba_io[2] = (lba & 0x0FF0000) >> 16;
        lba_io[3] = 0;
        lba_io[4] = 0;
        lba_io[5] = 0;
        head = (lba & 0xF000000) >> 24;
    } else {
        // CHS:
        lba_mode = 0;
        sect = (lba % 63) + 1;
        cyl = (lba + 1 - sect) / (16 * 63);
        lba_io[0] = sect;
        lba_io[1] = (cyl >> 0) & 0xFF;
        lba_io[2] = (cyl >> 8) & 0xFF;
        lba_io[3] = 0;
        lba_io[4] = 0;
        lba_io[5] = 0;
        head = (lba + 1 - sect) % (16 * 63) / (63);
    }

    // (II) See if drive supports DMA or not;
    dma = 0; // We don't support DMA

    // (III) Wait if the drive is busy;
    while (ide_read(channel, ATA_REG_STATUS) & ATA_SR_BSY)
        ; // Wait if busy.

    // (IV) Select Drive from the controller;
    if (lba_mode == 0)
        ide_write(channel, ATA_REG_HDDEVSEL, 0xA0 | (slavebit << 4) | head); // Drive & CHS.
    else
        ide_write(channel, ATA_REG_HDDEVSEL, 0xE0 | (slavebit << 4) | head); // Drive & LBA

    // (V) Write Parameters;
    if (lba_mode == 2) {
        ide_write(channel, ATA_REG_SECCOUNT1, 0);
        ide_write(channel, ATA_REG_LBA3, lba_io[3]);
        ide_write(channel, ATA_REG_LBA4, lba_io[4]);
        ide_write(channel, ATA_REG_LBA5, lba_io[5]);
    }
    ide_write(channel, ATA_REG_SECCOUNT0, numsects);
    ide_write(channel, ATA_REG_LBA0, lba_io[0]);
        ide_write(channel, ATA_REG_LBA1, lba_io[1]);
    ide_write(channel, ATA_REG_LBA2, lba_io[2]);

    if (lba_mode == 0 && dma == 0 && numsects == 1) cmd = ATA_CMD_READ_PIO;
    if (lba_mode == 1 && dma == 0 && numsects == 1) cmd = ATA_CMD_READ_PIO;
    if (lba_mode == 2 && dma == 0 && numsects == 1) cmd = ATA_CMD_READ_PIO_EXT;
    if (lba_mode == 0 && dma == 1 && numsects == 1) cmd = ATA_CMD_READ_DMA;
    if (lba_mode == 1 && dma == 1 && numsects == 1) cmd = ATA_CMD_READ_DMA;
    if (lba_mode == 2 && dma == 1 && numsects == 1) cmd = ATA_CMD_READ_DMA_EXT;
    ide_write(channel, ATA_REG_COMMAND, cmd);               // Send the Command.

    if (dma) {
        // DMA Read.
        // [...]
    } else {
        // PIO Read.
        for (i = 0; i < numsects; i++) {
            err = ide_polling(channel, 1);
            if (err)
                return err; // Polling, set error and exit if there is.
            
            // Copy Data from Hard Disk to ES:EDI;
            asm("pushw %es");
            asm("movw %%ax, %%es" : : "a"(es));
            asm("rep insw" : : "c"(words), "d"(bus), "D"(edi)); // Receive Data.
            asm("popw %es");
            edi += (words*2);
        }
    }

    return 0; // Easy, isn't it?
}

// Write sectors (basic implementation)
uint8_t ide_write_sectors(uint8_t drive, uint8_t numsects, uint32_t lba, uint16_t es, uint32_t edi) {
    uint8_t lba_mode, dma, cmd;
    uint8_t lba_io[6];
    uint32_t channel = ide_devices[drive].channel;
    uint32_t slavebit = ide_devices[drive].drive;
    uint32_t bus = channels[channel].base;
    uint32_t words = 256;
    uint16_t cyl, i;
    uint8_t head, sect, err;

    // (I) Select one from LBA28, LBA48 or CHS;
    if (lba >= 0x10000000) {
        // LBA48:
        lba_mode = 2;
        lba_io[0] = (lba & 0x000000FF) >> 0;
        lba_io[1] = (lba & 0x0000FF00) >> 8;
        lba_io[2] = (lba & 0x00FF0000) >> 16;
        lba_io[3] = (lba & 0xFF000000) >> 24;
        lba_io[4] = 0;
        lba_io[5] = 0;
        head = 0;
    } else if (ide_devices[drive].capabilities & 0x200) {
        // LBA28:
        lba_mode = 1;
        lba_io[0] = (lba & 0x00000FF) >> 0;
        lba_io[1] = (lba & 0x000FF00) >> 8;
        lba_io[2] = (lba & 0x0FF0000) >> 16;
        lba_io[3] = 0;
        lba_io[4] = 0;
        lba_io[5] = 0;
        head = (lba & 0xF000000) >> 24;
    } else {
        // CHS:
        lba_mode = 0;
        sect = (lba % 63) + 1;
        cyl = (lba + 1 - sect) / (16 * 63);
        lba_io[0] = sect;
        lba_io[1] = (cyl >> 0) & 0xFF;
        lba_io[2] = (cyl >> 8) & 0xFF;
        lba_io[3] = 0;
        lba_io[4] = 0;
        lba_io[5] = 0;
        head = (lba + 1 - sect) % (16 * 63) / (63);
    }

    // (II) See if drive supports DMA or not;
    dma = 0; // We don't support DMA

    // (III) Wait if the drive is busy;
    while (ide_read(channel, ATA_REG_STATUS) & ATA_SR_BSY)
        ; // Wait if busy.

    // (IV) Select Drive from the controller;
    if (lba_mode == 0)
        ide_write(channel, ATA_REG_HDDEVSEL, 0xA0 | (slavebit << 4) | head); // Drive & CHS.
    else
        ide_write(channel, ATA_REG_HDDEVSEL, 0xE0 | (slavebit << 4) | head); // Drive & LBA

    // (V) Write Parameters;
    if (lba_mode == 2) {
        ide_write(channel, ATA_REG_SECCOUNT1, 0);
        ide_write(channel, ATA_REG_LBA3, lba_io[3]);
        ide_write(channel, ATA_REG_LBA4, lba_io[4]);
        ide_write(channel, ATA_REG_LBA5, lba_io[5]);
    }
    ide_write(channel, ATA_REG_SECCOUNT0, numsects);
    ide_write(channel, ATA_REG_LBA0, lba_io[0]);
    ide_write(channel, ATA_REG_LBA1, lba_io[1]);
    ide_write(channel, ATA_REG_LBA2, lba_io[2]);

    if (lba_mode == 0 && dma == 0 && numsects == 1) cmd = ATA_CMD_WRITE_PIO;
    if (lba_mode == 1 && dma == 0 && numsects == 1) cmd = ATA_CMD_WRITE_PIO;
    if (lba_mode == 2 && dma == 0 && numsects == 1) cmd = ATA_CMD_WRITE_PIO_EXT;
    if (lba_mode == 0 && dma == 1 && numsects == 1) cmd = ATA_CMD_WRITE_DMA;
    if (lba_mode == 1 && dma == 1 && numsects == 1) cmd = ATA_CMD_WRITE_DMA;
    if (lba_mode == 2 && dma == 1 && numsects == 1) cmd = ATA_CMD_WRITE_DMA_EXT;
    ide_write(channel, ATA_REG_COMMAND, cmd);               // Send the Command.

    if (dma) {
        // DMA Write.
        // [...]
    } else {
        // PIO Write.
        for (i = 0; i < numsects; i++) {
            err = ide_polling(channel, 1);
            if (err)
                return err; // Polling, set error and exit if there is.
            
            // Copy Data from ES:EDI to Hard Disk;
            asm("pushw %es");
            asm("movw %%ax, %%es" : : "a"(es));
            asm("rep outsw" : : "c"(words), "d"(bus), "S"(edi)); // Send Data
            asm("popw %es");
            edi += (words*2);
        }
    }
    
    ide_write(channel, ATA_REG_COMMAND, (char []) {ATA_CMD_CACHE_FLUSH, ATA_CMD_CACHE_FLUSH,
                                                   ATA_CMD_CACHE_FLUSH_EXT, ATA_CMD_CACHE_FLUSH_EXT}[lba_mode]);
    ide_polling(channel, 0); // Polling.

    return 0; // Easy, isn't it?
}

int disk_flush_cache(int drive_index) {
    if (drive_index < 0 || drive_index >= 4) {
        return -1; // Invalid drive index
    }
    
    ide_device_t* drive = &ide_devices[drive_index];
    
    if (!drive->reserved) {
        return -1; // Drive not present
    }
    
    // Only flush cache for ATA drives
    if (drive->type != IDE_ATA) {
        return 0; // ATAPI drives don't need cache flush
    }
    
    // Get the correct channel base address using your existing constants
    uint16_t base;
    if (drive->channel == ATA_PRIMARY) {
        base = 0x1F0; // Primary ATA channel base
    } else {
        base = 0x170; // Secondary ATA channel base
    }
    
    // Select the drive
    outb(base + ATA_REG_HDDEVSEL, 0xA0 | (drive->drive << 4));
    
    // Wait for drive to be ready
    while (inb(base + ATA_REG_STATUS) & ATA_SR_BSY);
    
    // Send FLUSH CACHE command (0xE7)
    outb(base + ATA_REG_COMMAND, 0xE7);
    
    // Wait for command completion
    while (inb(base + ATA_REG_STATUS) & ATA_SR_BSY);
    
    // Check for errors
    uint8_t status = inb(base + ATA_REG_STATUS);
    if (status & ATA_SR_ERR) {
        return -1; // Error occurred
    }
    
    return 0; // Success
}

// IRQ handlers
void ide_wait_irq(void) {
    while (!ide_irq_invoked)
        ;
    ide_irq_invoked = 0;
}

void ide_irq(void) {
    ide_irq_invoked = 1;
}
