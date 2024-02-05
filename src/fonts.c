#include <uefi.h>
#include "graphics.h"
#include "string.h"

#define SSFN_IMPLEMENTATION
#include "ssfn.h"

ssfn_buf_t dst = {0};
ssfn_t ctx = {0};

int position[2] = {0, 0};

ssfn_font_t *font;

int char_width() {
	int w;
	int h;
	int left;
	int top;

	ssfn_bbox(&ctx, " ", &w, &h, &left, &top);
	return w;
}

void print(char *s) {

	int w;
	int h;
	int left;
	int top;

	ssfn_bbox(&ctx, " ", &w, &h, &left, &top);

	int ret;
	dst.x = position[0]+5;
	dst.y = position[1];
	while((ret = ssfn_render(&ctx, &dst, s)) > 0) {
		if (*s == '\n') {
			position[1] += ctx.size;
			position[0] = 0;
		} else {
			position[0] += char_width();
		}
		s += ret;
	}

}

void set_cursor_position(int x, int y) {
	position[0] = x;
	position[1] = y;
}

int load_font(char *s, efi_gop_t *gop) {
	FILE *f;
    long int size;

	f = fopen(s, "r");
	if (!f) {
		return -1;
	}
	fseek(f, 0, SEEK_END);
	size = ftell(f);
	fseek(f, 0, SEEK_SET);
	font = (ssfn_font_t*)malloc(size + 1);
	if(!font) {
		return -2;
	}
	fread(font, size, 1, f);
	fclose(f);
	ssfn_load(&ctx, font);

	dst.ptr = (unsigned char*)gop->Mode->FrameBufferBase;
	dst.w = gop->Mode->Information->HorizontalResolution;
	dst.h = gop->Mode->Information->VerticalResolution;
	dst.p = sizeof(unsigned int) * gop->Mode->Information->PixelsPerScanLine;
	dst.fg = 0xFFFFFFFF;

	ssfn_select(&ctx, SSFN_FAMILY_ANY, NULL, SSFN_STYLE_REGULAR | SSFN_STYLE_NOCACHE, max(gop->Mode->Information->VerticalResolution/55, 20));
	position[1] = ctx.size;

	return 0;

}

void free_font() {
    ssfn_free(&ctx);
    free(font);
}

ssfn_t* get_ctx() {
	return &ctx;
}

void get_cursor_position(int* pos) {
	pos[0] = position[0];
	pos[1] = position[1];
}

void clear_char(int x, int y) {

	int w;
	int h;
	int left;
	int top;

	ssfn_bbox(&ctx, " ", &w, &h, &left, &top);

	draw_rectangle(x+4, y-ctx.size+4, w, h, 0, 0, 0);

}

void convert_to_pixels(const int* src, int* dst) {

	int w;
	int h;
	int left;
	int top;

	ssfn_bbox(&ctx, " ", &w, &h, &left, &top);

	dst[0] = src[0]*w;
	dst[1] = src[1]*ctx.size;

}

void convert_to_coords(const int* src, int* dst) {

	int w;
	int h;
	int left;
	int top;

	ssfn_bbox(&ctx, " ", &w, &h, &left, &top);

	dst[0] = src[0]/w;
	dst[1] = src[1]/ctx.size;

}

void set_fg(int hex) {
	dst.fg = hex;
}

void info_prefix() {
	print("[");
	set_fg(0xFF00FF00);
	print("INFO");
	set_fg(0xFFFFFFFF);
	print("] ");
}
