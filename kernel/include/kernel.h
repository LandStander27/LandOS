#ifndef KERNEL_H
#define KERNEL_H

#include <uefi.h>

typedef struct {
	efi_physical_address_t framebuffer_base;
	uint32_t width;
	uint32_t height;
	uint32_t pixels_per_scanline;
} graphics_information;

int kernel_main(graphics_information* gop);

#endif