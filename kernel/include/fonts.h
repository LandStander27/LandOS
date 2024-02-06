#ifndef FONTS_H
#define FONTS_H

#include "ssfn.h"
#include <uefi.h>

void print(const char *s);
int load_font(char *s, efi_physical_address_t framebuffer_base, uint32_t pixels_per_scanline, uint32_t width, uint32_t height);
void free_font();
ssfn_t* get_ctx();
void set_cursor_position(int x, int y);
int* get_cursor_position();
void clear_char(int x, int y);
// void convert_to_pixels(const int* src, int* dst);
// void convert_to_coords(const int* src, int* dst);
void set_fg(int hex);
void info_prefix();
void clear_buffer();
void init_buffer();

#endif