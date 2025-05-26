# Compiler and tools
CC = i686-elf-gcc
AS = i686-elf-as
LD = i686-elf-gcc

# Compiler flags
CFLAGS = -std=gnu99 -ffreestanding -O2 -Wall -Wextra -Isrc/include
LDFLAGS = -T src/linker.ld -ffreestanding -O2 -nostdlib -lgcc

# Directories
SRCDIR = src
BUILDDIR = build
BOOTDIR = $(BUILDDIR)/bootloader
KERNELDIR = $(BUILDDIR)/kernel
DRIVERSDIR = $(BUILDDIR)/drivers
COMMANDSDIR = $(BUILDDIR)/commands

# Source files
BOOT_SOURCES = $(wildcard $(SRCDIR)/bootloader/*.s)
KERNEL_SOURCES = $(wildcard $(SRCDIR)/kernel/*.c)
DRIVER_SOURCES = $(wildcard $(SRCDIR)/drivers/*.c)
COMMAND_SOURCES = $(wildcard $(SRCDIR)/commands/*.c)

# Object files
BOOT_OBJECTS = $(BOOT_SOURCES:$(SRCDIR)/bootloader/%.s=$(BOOTDIR)/%.o)
KERNEL_OBJECTS = $(KERNEL_SOURCES:$(SRCDIR)/kernel/%.c=$(KERNELDIR)/%.o)
DRIVER_OBJECTS = $(DRIVER_SOURCES:$(SRCDIR)/drivers/%.c=$(DRIVERSDIR)/%.o)
COMMAND_OBJECTS = $(COMMAND_SOURCES:$(SRCDIR)/commands/%.c=$(COMMANDSDIR)/%.o)

# All object files
OBJECTS = $(COMMAND_OBJECTS) $(DRIVER_OBJECTS) $(KERNEL_OBJECTS) $(BOOT_OBJECTS)

# Target
TARGET = $(BUILDDIR)/SyncWideOS.bin
ISO_TARGET = $(BUILDDIR)/SyncWideOS.iso

# Default target - build both binary and ISO
all: $(ISO_TARGET)

# Create directories
$(BOOTDIR) $(KERNELDIR) $(DRIVERSDIR) $(COMMANDSDIR):
	mkdir -p $@

# Compile bootloader assembly files
$(BOOTDIR)/%.o: $(SRCDIR)/bootloader/%.s | $(BOOTDIR)
	$(AS) $< -o $@

# Compile kernel C files
$(KERNELDIR)/%.o: $(SRCDIR)/kernel/%.c | $(KERNELDIR)
	$(CC) -c $< -o $@ $(CFLAGS)

# Compile driver C files
$(DRIVERSDIR)/%.o: $(SRCDIR)/drivers/%.c | $(DRIVERSDIR)
	$(CC) -c $< -o $@ $(CFLAGS)

# Compile command C files
$(COMMANDSDIR)/%.o: $(SRCDIR)/commands/%.c | $(COMMANDSDIR)
	$(CC) -c $< -o $@ $(CFLAGS)

# Generate file data from iso_files
src/generated/iso_files_data.c: scripts/generate_files.sh
	@echo "Generating file data from iso_files..."
	@mkdir -p src/generated
	@./scripts/generate_files.sh

# Add the generated file to objects
GENERATED_OBJECTS = build/generated/iso_files_data.o

build/generated/iso_files_data.o: src/generated/iso_files_data.c
	@mkdir -p build/generated
	$(CC) $(CFLAGS) -c $< -o $@

# Update your main target to include generated objects
$(TARGET): $(OBJECTS) $(GENERATED_OBJECTS)
	$(LD) $(LDFLAGS) -o $@ $^

# Create ISO
$(ISO_TARGET): $(TARGET) src/generated/iso_files_data.c
	rm -rf isodir
	mkdir -p isodir/boot/grub
	cp $(TARGET) isodir/boot/SyncWideOS.bin
	cp src/grub.cfg isodir/boot/grub/grub.cfg
	@if [ -d iso_files ]; then \
	    echo "Copying files from iso_files to ISO..."; \
	    cp -r iso_files/* isodir/; \
	    echo "Files included in ISO:"; \
	    find isodir -name "*.txt" -o -name "*.ini" -o -name "*.cfg" | grep -v boot; \
	fi
	grub-mkrescue -o $(ISO_TARGET) isodir

# Clean build files
clean:
	rm -rf build/
	rm -rf isodir/boot/
	rm -f isodir/*.bin

.PHONY: all clean
