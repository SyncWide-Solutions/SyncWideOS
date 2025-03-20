ASM=nasm

SRC_DIR=src
BUILD_DIR=build
ISO_NAME=syncwideos.iso

.PHONY: all iso_image kernel bootloader clean always

#
# ISO image
#
all: iso_image

iso_image: $(BUILD_DIR)/$(ISO_NAME)

$(BUILD_DIR)/$(ISO_NAME): bootloader kernel
	mkdir -p $(BUILD_DIR)/iso
	cp $(BUILD_DIR)/bootloader.bin $(BUILD_DIR)/iso/
	cp $(BUILD_DIR)/kernel.bin $(BUILD_DIR)/iso/
	genisoimage -o $(BUILD_DIR)/$(ISO_NAME) -b bootloader.bin -no-emul-boot -boot-load-size 4 -boot-info-table -iso-level 3 -J -R $(BUILD_DIR)/iso/

#
# Bootloader
#
bootloader: $(BUILD_DIR)/bootloader.bin

$(BUILD_DIR)/bootloader.bin: always
	$(ASM) $(SRC_DIR)/bootloader/boot.asm -f bin -o $(BUILD_DIR)/bootloader.bin

#
# Kernel
#
kernel: $(BUILD_DIR)/kernel.bin

$(BUILD_DIR)/kernel.bin: always
	$(ASM) $(SRC_DIR)/kernel/main.asm -f bin -o $(BUILD_DIR)/kernel.bin

#
# Always
#
always:
	mkdir -p $(BUILD_DIR)

#
# Clean
#
clean:
	rm -rf $(BUILD_DIR)/*

#
# Run
#
run:
	qemu-system-x86_64 -cdrom $(BUILD_DIR)/$(ISO_NAME)