#include <uefi.h>
#include <stdbool.h>
#include "stdlib.c"
#include "time.c"
#include "fonts.h"
#include "graphics.h"
#include "shell.h"

efi_gop_t *gop = NULL;
efi_gop_mode_info_t *gop_info = NULL;

#define white 255, 255, 255
#define black 0, 0, 0
#define ms * 1000

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

void poll() {
	while (true) {
		sleep(1);
	};
}

int main(int argc, char **argv) {
	efi_status_t status;
	efi_guid_t gop_guid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
	uintn_t isiz = sizeof(efi_gop_mode_info_t);

	ST->ConOut->ClearScreen(ST->ConOut);

	info_prefixf();
	printf("setting random seed\n");

	struct tm* t = localtime(NULL);
	srand(t->tm_sec*t->tm_hour*t->tm_min*t->tm_year);

	info_prefixf();
	printf("getting gop\n");

	status = BS->LocateProtocol(&gop_guid, NULL, (void**)&gop);
	if (EFI_ERROR(status) || !gop) {
		err_prefixf();
		printf("unable to get graphics output protocol\n");
		poll();
	}

	info_prefixf();
	printf("getting current video mode\n");

	status = gop->QueryMode(gop, gop->Mode ? gop->Mode->Mode : 0, &isiz, &gop_info);
	if(status == EFI_NOT_STARTED || !gop->Mode) {
		status = gop->SetMode(gop, 0);
	}
	if(EFI_ERROR(status)) {
		err_prefixf();
		printf("unable to get current video mode\n");
		poll();
	}

	ST->ConOut->ClearScreen(ST->ConOut);
	int selected = -1;
	for (int i = 0; i < gop->Mode->MaxMode; i++) {
		status = gop->QueryMode(gop, i, &isiz, &gop_info);
		if (EFI_ERROR(status)) {
			continue;
		}
		if (gop_info->HorizontalResolution == 1920 && gop_info->VerticalResolution == 1080) {
			selected = i;
		}
		if (i == selected) {
			ST->ConOut->SetAttribute(ST->ConOut, EFI_BACKGROUND_LIGHTGRAY | EFI_BLACK);
		}
		printf("  %4d   %4d\n", gop_info->HorizontalResolution, gop_info->VerticalResolution);
		ST->ConOut->SetAttribute(ST->ConOut, EFI_BACKGROUND_BLACK | EFI_LIGHTGRAY);
	}

	if (selected == -1) {
		selected = 0;
	}

	ST->ConOut->SetCursorPosition(ST->ConOut, 0, selected);
	gop->QueryMode(gop, selected+1, &isiz, &gop_info);
	ST->ConOut->SetAttribute(ST->ConOut, EFI_BACKGROUND_LIGHTGRAY | EFI_BLACK);
	printf("  %4d   %4d\n", gop_info->HorizontalResolution, gop_info->VerticalResolution);
	ST->ConOut->SetAttribute(ST->ConOut, EFI_BACKGROUND_BLACK | EFI_LIGHTGRAY);

	efi_input_key_t key;
	while (true) {
		status = ST->ConIn->ReadKeyStroke(ST->ConIn, &key);
		if (key.ScanCode == 0x2) {

			if (selected == min(29, gop->Mode->MaxMode)) {
				continue;
			}

			ST->ConOut->SetCursorPosition(ST->ConOut, 0, selected);
			gop->QueryMode(gop, selected, &isiz, &gop_info);
			printf("  %4d   %4d\n", gop_info->HorizontalResolution, gop_info->VerticalResolution);

			ST->ConOut->SetCursorPosition(ST->ConOut, 0, selected+1);
			gop->QueryMode(gop, selected+1, &isiz, &gop_info);
			ST->ConOut->SetAttribute(ST->ConOut, EFI_BACKGROUND_LIGHTGRAY | EFI_BLACK);
			printf("  %4d   %4d\n", gop_info->HorizontalResolution, gop_info->VerticalResolution);
			ST->ConOut->SetAttribute(ST->ConOut, EFI_BACKGROUND_BLACK | EFI_LIGHTGRAY);

			selected++;
		} else if (key.ScanCode == 0x1) {

			if (selected == 0) {
				continue;
			}

			ST->ConOut->SetCursorPosition(ST->ConOut, 0, selected);
			gop->QueryMode(gop, selected, &isiz, &gop_info);
			printf("  %4d   %4d\n", gop_info->HorizontalResolution, gop_info->VerticalResolution);

			ST->ConOut->SetCursorPosition(ST->ConOut, 0, selected-1);
			gop->QueryMode(gop, selected-1, &isiz, &gop_info);
			ST->ConOut->SetAttribute(ST->ConOut, EFI_BACKGROUND_LIGHTGRAY | EFI_BLACK);
			printf("  %4d   %4d\n", gop_info->HorizontalResolution, gop_info->VerticalResolution);
			ST->ConOut->SetAttribute(ST->ConOut, EFI_BACKGROUND_BLACK | EFI_LIGHTGRAY);

			selected--;
		} else {
			status = ST->ConIn->ReadKeyStroke(ST->ConIn, &key);
			if (key.UnicodeChar == 0x0D) {
				break;
			} else {
				continue;
			}
		}
		usleep(100);
	}

	ST->ConOut->ClearScreen(ST->ConOut);

	info_prefixf();
	printf("setting video mode\n");

	status = gop->SetMode(gop, selected);
	if (EFI_ERROR(status)) {
		err_prefixf();
		printf("unable to set video mode\n");
		poll();
	}

	status = gop->QueryMode(gop, gop->Mode->Mode, &isiz, &gop_info);
	info_prefixf();
	printf("Currently loaded video mode: %d, %d\n", gop_info->HorizontalResolution, gop_info->VerticalResolution);

	info_prefixf();
	printf("loading font\n");

	int e;
	if ((e = load_font("\\fonts\\MesloLGS NF.sfn", gop)) != 0) {
		if (e == -1) {
			err_prefixf();
			printf("could not open font\n");
		} else if (e == -2) {
			err_prefixf();
			printf("unable to allocate memory for font\n");
		}
		poll();
	}

	info_prefix();
	print("font loaded\n");
	info_prefix();
	print("loading shell\n");

	init_graphics(gop->Mode->FrameBufferBase, gop->Mode->Information->PixelsPerScanLine, gop->Mode->Information->HorizontalResolution, gop->Mode->Information->HorizontalResolution);
	usleep(500 ms);
	for (int x = 0; x < gop_info->HorizontalResolution; x++) {
		for (int y = 0; y < gop_info->VerticalResolution; y++) {
			put_pixel(x, y, black);
			if (y % 20 == 0) {
				usleep(1);
			}
		}
	}

	set_cursor_position(0, get_ctx()->size);
	int code = shell();
	usleep(250 ms);

	info_prefix();
	print("unallocating font\n");
	usleep(250 ms);
	free_font();

	info_prefix();
	print("executing instruction\n");
	usleep(250 ms);

	if (code == 1) {
		ST->RuntimeServices->ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0, 0);
	} else if (code == 2) {
		ST->RuntimeServices->ResetSystem(EfiResetCold, EFI_SUCCESS, 0, 0);
	}

	poll();

	return 0;
}
