#include <uefi.h>

efi_physical_address_t base = 0;
uint32_t per_scan_line = 0;
int width;
int height;

void put_pixel(int x, int y, int r, int g, int b) {
	*((uint32_t*)(base + 4 * per_scan_line * y + 4 * x)) = b;
	*((uint32_t*)(base + 4 * per_scan_line * y + 4 * x + 1)) = g;
	*((uint32_t*)(base + 4 * per_scan_line * y + 4 * x + 2)) = r;
	*((uint32_t*)(base + 4 * per_scan_line * y + 4 * x + 3)) = 255;
}

void get_pixel(int x, int y, int *r, int *g, int *b) {
	*b = *((uint32_t*)(base + 4 * per_scan_line * y + 4 * x));
	*g = *((uint32_t*)(base + 4 * per_scan_line * y + 4 * x + 1));
	*r = *((uint32_t*)(base + 4 * per_scan_line * y + 4 * x + 2));
}

void draw_rectangle(int x, int y, int w, int h, int r, int g, int b) {
	for (int i = x; i < x+w; i++) {
		for (int j = y; j < y+h; j++) {
			put_pixel(i, j, r, g, b);
		}
	}
}

void clear_screen() {
	for (int i = 0; i < width; i++) {
		for (int j = 0; j < height; j++) {
			put_pixel(i, j, 0, 0, 0);
		}
	}
}

void init_graphics(efi_physical_address_t buffer_base, uint32_t pixels_per_scan_line, int w, int h) {
	base = buffer_base;
	per_scan_line = pixels_per_scan_line;
	width = w;
	height = h;
}
