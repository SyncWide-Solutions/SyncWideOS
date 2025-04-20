BUILD_DIR := build

# Create build directory if it doesn't exist
$(shell mkdir -p $(BUILD_DIR))

all: grub

boot: 
	i686-elf-as boot.s -o $(BUILD_DIR)/boot.o

kernel: kernel.c
	i686-elf-gcc -c kernel.c -o $(BUILD_DIR)/kernel.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra

link: boot kernel
	i686-elf-gcc -T linker.ld -o $(BUILD_DIR)/myos.bin -ffreestanding -O2 -nostdlib $(BUILD_DIR)/boot.o $(BUILD_DIR)/kernel.o -lgcc

grub: link
	grub-file --is-x86-multiboot $(BUILD_DIR)/myos.bin
	mkdir -p $(BUILD_DIR)/isodir/boot/grub
	cp $(BUILD_DIR)/myos.bin $(BUILD_DIR)/isodir/boot/myos.bin
	cp grub.cfg $(BUILD_DIR)/isodir/boot/grub/grub.cfg
	grub-mkrescue -o $(BUILD_DIR)/myos.iso $(BUILD_DIR)/isodir

clean:
	rm -rf $(BUILD_DIR)

# Please don't edit this
run:
	qemu-system-x86_64 \
	-fda $(BUILD_DIR)/myos.iso \
	-net nic \
	-net user

test:
	timeout 20s qemu-system-x86_64 \
		-drive file=build/myos.iso,format=raw,if=floppy \
		-net nic \
		-net user \
		-nographic \
		-serial mon:stdio \
		-display none \
		-no-reboot | tee qemu.log ; \
		if grep -q "SyncWide OS version" qemu.log ; then echo "Boot success"; else echo "Boot failed"; exit 1; fi
