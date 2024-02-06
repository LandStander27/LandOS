#include <uefi.h>
#include "graphics.h"
#include "fonts.h"
#include "shell.h"
#include "ssfn.h"

#define ms * 1000

typedef struct {
	efi_physical_address_t framebuffer_base;
	uint32_t width;
	uint32_t height;
	uint32_t pixels_per_scanline;
} graphics_information;

static inline void info_prefixf() {
	printf("[");
	ST->ConOut->SetAttribute(ST->ConOut, EFI_GREEN);
	printf("INFO");
	ST->ConOut->SetAttribute(ST->ConOut, EFI_LIGHTGRAY);
	printf("] ");
}

static inline void err_prefixf() {
	printf("[");
	ST->ConOut->SetAttribute(ST->ConOut, EFI_RED);
	printf("ERR ");
	ST->ConOut->SetAttribute(ST->ConOut, EFI_LIGHTGRAY);
	printf("] ");
}

int kernel_main(graphics_information* gop) {

	printf("wadawdawd");
	printf("loading font\n");

	int e;
	if ((e = load_font("\\fonts\\MesloLGS NF.sfn", gop->framebuffer_base, gop->pixels_per_scanline, gop->width, gop->height)) != 0) {
		if (e == -1) {
			err_prefixf();
			printf("could not open font\n");
		} else if (e == -2) {
			err_prefixf();
			printf("unable to allocate memory for font\n");
		}
		return -1;
	}

	info_prefix();
	print("allocating screen buffer\n");
	init_buffer();
	info_prefix();
	print("init graphics\n");

	init_graphics(gop->framebuffer_base, gop->pixels_per_scanline, gop->width, gop->height);
	usleep(500 ms);
	clear_buffer();

	set_cursor_position(0, 1);
	int code = shell();
	usleep(250 ms);

	info_prefix();
	print("unallocating screen data\n");
	usleep(500 ms);
	free_font();

	info_prefix();
	printf("executing instruction\n");
	usleep(500 ms);

	if (code == 1) {
		ST->RuntimeServices->ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0, 0);
	} else if (code == 2) {
		ST->RuntimeServices->ResetSystem(EfiResetCold, EFI_SUCCESS, 0, 0);
	} else if (code == 3) {
		ST->RuntimeServices->ResetSystem(EfiResetWarm, EFI_SUCCESS, 0, 0);
	}

	return 0;
}
