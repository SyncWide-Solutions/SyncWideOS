ASM=nasm
SRC_DIR=src
BUILD_DIR=build
KERNEL_SRC=$(SRC_DIR)/kernel
TOOLS_SRC=$(SRC_DIR)/tools

.PHONY: all floppy_image kernel bootloader clean always

all: floppy_image

floppy_image: $(BUILD_DIR)/main_floppy.img

$(BUILD_DIR)/main_floppy.img: bootloader kernel
	dd if=/dev/zero of=$(BUILD_DIR)/main_floppy.img bs=512 count=2880
	mkfs.fat -F 12 -n "SWOS" $(BUILD_DIR)/main_floppy.img
	dd if=$(BUILD_DIR)/bootloader.bin of=$(BUILD_DIR)/main_floppy.img conv=notrunc
	cp $(BUILD_DIR)/kernel.bin $(BUILD_DIR)/main_floppy.img

bootloader: $(BUILD_DIR)/bootloader.bin

$(BUILD_DIR)/bootloader.bin: always
	$(ASM) $(SRC_DIR)/bootloader/boot.asm -f bin -o $(BUILD_DIR)/bootloader.bin

kernel: $(BUILD_DIR)/kernel.bin

$(BUILD_DIR)/kernel.bin: always
	$(ASM) $(KERNEL_SRC)/main.asm -f bin -o $(BUILD_DIR)/kernel.bin

always:
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)/*

# Please don't edit this
run:
	qemu-system-x86_64 \
	-fda $(BUILD_DIR)/main_floppy.img \
	-net nic \
	-net user

test:
	timeout 20s qemu-system-x86_64 \
		-drive file=build/main_floppy.img,format=raw,if=floppy \
		-net nic \
		-net user \
		-nographic \
		-serial mon:stdio \
		-display none \
		-no-reboot | tee qemu.log ; \
		if grep -q "SyncWide OS version" qemu.log ; then echo "Boot success"; else echo "Boot failed"; exit 1; fi
