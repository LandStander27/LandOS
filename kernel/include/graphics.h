#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <uefi.h>

void put_pixel(int x, int y, int r, int g, int b);
void init_graphics(efi_physical_address_t buffer_base, uint32_t pixels_per_scan_line, int w, int h);
void draw_rectangle(int x, int y, int w, int h, int r, int g, int b);
void clear_screen();
void get_pixel(int x, int y, int *r, int *g, int *b);

#endif