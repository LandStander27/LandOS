TARGET = BOOTX64.EFI
SRCS = $(wildcard *src/*.c)
CFLAGS = -Iinclude
USE_GCC=1
include uefi/Makefile

run:
	mkdir -p bin
	mv BOOTX64.efi bin
	mv *src/*.o bin
	mdel -i boot/uefi.img ::/EFI/BOOT/BOOTX64.efi
	mcopy -i boot/uefi.img bin/BOOTX64.EFI ::/EFI/BOOT
	xorriso -as mkisofs -R -f -e uefi.img -no-emul-boot -o boot.iso boot
	qemu-system-x86_64 -bios /usr/share/ovmf/OVMF.fd -cdrom boot.iso

build:
	mkdir -p bin
	mv BOOTX64.efi bin
	mv *src/*.o bin
	mdel -i boot/uefi.img ::/EFI/BOOT/BOOTX64.efi
	mcopy -i boot/uefi.img bin/BOOTX64.EFI ::/EFI/BOOT
	xorriso -as mkisofs -R -f -e uefi.img -no-emul-boot -o boot.iso boot

clean:
	rm -rf bin boot.o
