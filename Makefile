BUILD_DIR := build
SRC_DIR := src

# Create build directory if it doesn't exist
$(shell mkdir -p $(BUILD_DIR))
$(shell mkdir -p $(BUILD_DIR)/commands)

all: grub

boot:
	i686-elf-as $(SRC_DIR)/bootloader/boot.s -o $(BUILD_DIR)/boot.o

# Define command sources and objects
COMMAND_SOURCES := $(wildcard $(SRC_DIR)/commands/*.c)
COMMAND_OBJECTS := $(patsubst $(SRC_DIR)/commands/%.c,$(BUILD_DIR)/commands/%.o,$(COMMAND_SOURCES))

# Target to build all command objects
commands: $(COMMAND_OBJECTS)

# Pattern rule to compile each command file individually
$(BUILD_DIR)/commands/%.o: $(SRC_DIR)/commands/%.c
	i686-elf-gcc -c $< -o $@ -std=gnu99 -ffreestanding -O2 -Wall -Wextra -I$(SRC_DIR)/include

filesystem: $(SRC_DIR)/kernel/filesystem.c
	i686-elf-gcc -c $(SRC_DIR)/kernel/filesystem.c -o $(BUILD_DIR)/filesystem.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra -I$(SRC_DIR)/include

string: $(SRC_DIR)/kernel/string.c
	i686-elf-gcc -c $(SRC_DIR)/kernel/string.c -o $(BUILD_DIR)/string.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra -I$(SRC_DIR)/include

kernel: $(SRC_DIR)/kernel/kernel.c
	i686-elf-gcc -c $(SRC_DIR)/kernel/kernel.c -o $(BUILD_DIR)/kernel.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra -I$(SRC_DIR)/include

link: boot kernel commands filesystem string
	i686-elf-gcc -T $(SRC_DIR)/linker.ld -o $(BUILD_DIR)/myos.bin -ffreestanding -O2 -nostdlib $(BUILD_DIR)/boot.o $(BUILD_DIR)/kernel.o $(BUILD_DIR)/filesystem.o $(BUILD_DIR)/string.o $(BUILD_DIR)/commands/*.o -lgcc

grub: link
	grub-file --is-x86-multiboot $(BUILD_DIR)/myos.bin
	mkdir -p $(BUILD_DIR)/isodir/boot/grub
	cp $(BUILD_DIR)/myos.bin $(BUILD_DIR)/isodir/boot/myos.bin
	cp $(SRC_DIR)/grub.cfg $(BUILD_DIR)/isodir/boot/grub/grub.cfg
	grub-mkrescue -o $(BUILD_DIR)/myos.iso $(BUILD_DIR)/isodir

build_floppy: link
	dd if=/dev/zero of=$(BUILD_DIR)/floppy.img bs=512 count=2880
	mkfs.vfat -F 12 $(BUILD_DIR)/floppy.img
	mkdir -p $(BUILD_DIR)/mnt
	sudo mount -o loop $(BUILD_DIR)/floppy.img $(BUILD_DIR)/mnt
	sudo mkdir -p $(BUILD_DIR)/mnt/boot
	sudo cp $(BUILD_DIR)/myos.bin $(BUILD_DIR)/mnt/boot/
	echo 'DEFAULT myos' | sudo tee $(BUILD_DIR)/mnt/syslinux.cfg
	echo 'LABEL myos' | sudo tee -a $(BUILD_DIR)/mnt/syslinux.cfg
	echo '  KERNEL /boot/myos.bin' | sudo tee -a $(BUILD_DIR)/mnt/syslinux.cfg
	sudo umount $(BUILD_DIR)/mnt
	syslinux -i $(BUILD_DIR)/floppy.img
	rmdir $(BUILD_DIR)/mnt

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
