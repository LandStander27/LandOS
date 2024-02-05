#ifndef FONTS_H
#define FONTS_H

#include "ssfn.h"

void print(char *s);
int load_font(char *s, efi_gop_t *gop);
void free_font();
ssfn_t* get_ctx();
void set_cursor_position(int x, int y);
void get_cursor_position(int* pos);
void clear_char(int x, int y);
void convert_to_pixels(const int* src, int* dst);
void convert_to_coords(const int* src, int* dst);
int char_width();
void set_fg(int hex);
void info_prefix();

#endif