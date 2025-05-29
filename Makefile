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

# Targets
TARGET = $(BUILDDIR)/SyncWideOS.bin
ISO_TARGET = $(BUILDDIR)/SyncWideOS.iso
IMG_TARGET = $(BUILDDIR)/SyncWideOS.img

# Default target - build ISO
all: $(ISO_TARGET)

# Build DD mode image
dd: $(IMG_TARGET)

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

# Link kernel binary
$(TARGET): $(OBJECTS) $(GENERATED_OBJECTS)
	$(LD) $(LDFLAGS) -o $@ $^

# Create GRUB config if it doesn't exist
src/grub.cfg:
	@echo "Creating default GRUB config..."
	@mkdir -p src
	@echo 'menuentry "SyncWideOS" {' > src/grub.cfg
	@echo '    multiboot /boot/SyncWideOS.bin' >> src/grub.cfg
	@echo '}' >> src/grub.cfg

# Create ISO for ISO mode (recommended)
$(ISO_TARGET): $(TARGET) src/generated/iso_files_data.c src/grub.cfg
	@echo "Creating bootable ISO..."
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
	@echo "ISO created: $(ISO_TARGET)"
	@echo "Write to USB with: sudo dd if=$(ISO_TARGET) of=/dev/sdX bs=1M status=progress"

# Create disk image for DD mode
$(IMG_TARGET): $(TARGET)
	@echo "Creating disk image for DD mode..."
	# Create 10MB disk image
	dd if=/dev/zero of=$(IMG_TARGET) bs=1M count=10
	# Create partition table
	echo -e "o\nn\np\n1\n\n\na\n1\nw\n" | fdisk $(IMG_TARGET)
	# Setup loop device
	sudo losetup -P /dev/loop0 $(IMG_TARGET) || true
	# Format partition
	sudo mkfs.fat -F32 /dev/loop0p1 || true
	# Mount and copy kernel
	mkdir -p mnt
	sudo mount /dev/loop0p1 mnt || true
	sudo cp $(TARGET) mnt/kernel.bin
	@if [ -d iso_files ]; then \
		sudo cp -r iso_files/* mnt/; \
	fi
	sudo umount mnt || true
	sudo losetup -d /dev/loop0 || true
	rmdir mnt
	@echo "Disk image created: $(IMG_TARGET)"
	@echo "Write to USB with: sudo dd if=$(IMG_TARGET) of=/dev/sdX bs=1M status=progress"

# Test in QEMU
test-iso: $(ISO_TARGET)
	qemu-system-i386 -cdrom $(ISO_TARGET) -m 512M

test-dd: $(IMG_TARGET)
	qemu-system-i386 -drive file=$(IMG_TARGET),format=raw -m 512M

# Clean build files
clean:
	rm -rf build/
	rm -rf isodir/
	rm -f src/generated/iso_files_data.c

# Help target
help:
	@echo "Available targets:"
	@echo "  all       - Build ISO image (default)"
	@echo "  dd        - Build disk image for DD mode"
	@echo "  test-iso  - Test ISO in QEMU"
	@echo "  test-dd   - Test disk image in QEMU"
	@echo "  clean     - Clean build files"
	@echo ""
	@echo "Usage:"
	@echo "  make           # Build ISO"
	@echo "  make dd        # Build DD image"
	@echo "  make test-iso  # Test in QEMU"

.PHONY: all dd clean test-iso test-dd help
