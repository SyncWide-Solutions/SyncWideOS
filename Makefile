BUILD_DIR := build
SRC_DIR := src

# Create build directory if it doesn't exist
$(shell mkdir -p $(BUILD_DIR))
$(shell mkdir -p $(BUILD_DIR)/commands)

all: grub

boot:
	i686-elf-as $(SRC_DIR)/bootloader/boot.s -o $(BUILD_DIR)/boot.o

# Compile commands
commands: $(wildcard $(SRC_DIR)/commands/*.c)
	for file in $^ ; do \
		i686-elf-gcc -c $$file -o $(BUILD_DIR)/commands/$$(basename $$file .c).o -std=gnu99 -ffreestanding -O2 -Wall -Wextra -I$(SRC_DIR)/include ; \
	done

kernel: $(SRC_DIR)/kernel/kernel.c
	i686-elf-gcc -c $(SRC_DIR)/kernel/kernel.c -o $(BUILD_DIR)/kernel.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra -I$(SRC_DIR)/include

link: boot kernel commands
	i686-elf-gcc -T $(SRC_DIR)/linker.ld -o $(BUILD_DIR)/myos.bin -ffreestanding -O2 -nostdlib $(BUILD_DIR)/boot.o $(BUILD_DIR)/kernel.o $(BUILD_DIR)/commands/*.o -lgcc

grub: link
	grub-file --is-x86-multiboot $(BUILD_DIR)/myos.bin
	mkdir -p $(BUILD_DIR)/isodir/boot/grub
	cp $(BUILD_DIR)/myos.bin $(BUILD_DIR)/isodir/boot/myos.bin
	cp $(SRC_DIR)/grub.cfg $(BUILD_DIR)/isodir/boot/grub/grub.cfg
	grub-mkrescue -o $(BUILD_DIR)/myos.iso $(BUILD_DIR)/isodir

build_floppy: link
	dd if=/dev/zero of=$(BUILD_DIR)/floppy.img bs=1474560 count=1
	dd if=$(BUILD_DIR)/myos.bin of=$(BUILD_DIR)/floppy.img conv=notrunc

clean:
	rm -rf $(BUILD_DIR)

run:
	qemu-system-x86_64 \
	-fda $(BUILD_DIR)/myos.iso \
	-net nic \
	-net user

run_floppy:
	qemu-system-x86_64 \
	-fda $(BUILD_DIR)/floppy.img \
	-net nic \
	-net user

test:
	timeout 20s qemu-system-x86_64 \
		-drive file=$(BUILD_DIR)/myos.iso,format=raw,if=floppy \
		-net nic \
		-net user \
		-nographic \
		-serial mon:stdio \
		-display none \
		-no-reboot | tee qemu.log ; \
		if grep -q "SyncWide OS version" qemu.log ; then echo "Boot success"; else echo "Boot failed"; exit 1; fi
