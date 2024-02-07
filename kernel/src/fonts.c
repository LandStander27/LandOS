#include <uefi.h>
#include "graphics.h"
#include "string.h"

#define SSFN_IMPLEMENTATION
#include "ssfn.h"

ssfn_buf_t dst = {0};
ssfn_t ctx = {0};

int position[2] = {0, 0};

#define CHAR_WIDTH 10
#define CHAR_HEIGHT 20

ssfn_font_t *font;

typedef struct {
    char character;
    uint32_t fg_color;
	uint32_t bg_color;
} text_entry_t;

static text_entry_t* terminal_data;

int terminal_width;
int terminal_height;

int renderc(int x, int y) {
	if (terminal_data[y*terminal_width+x].character == 0x00) {
		return 0;
	}
	dst.x = x*CHAR_WIDTH+5;
	dst.y = y*CHAR_HEIGHT;
	dst.fg = terminal_data[y*terminal_width+x].fg_color;

	char s[2] = { terminal_data[y*terminal_width+x].character, '\0' };
	int ret = ssfn_render(&ctx, &dst, s);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

int putc(int x, int y, char character, uint32_t fg_color, uint32_t bg_color) {

	terminal_data[y*terminal_width+x].character = character;
	terminal_data[y*terminal_width+x].fg_color = fg_color;
	terminal_data[y*terminal_width+x].bg_color = bg_color;

	renderc(x, y);

}

void rerender() {
	draw_rectangle(0, 0, terminal_width*CHAR_WIDTH, terminal_height*CHAR_HEIGHT, 0, 0, 0);
	for (int x = 0; x < terminal_width; x++) {
		for (int y = 0; y < terminal_height; y++) {
			renderc(x, y);
		}
	}
}

void scroll(int lines) {
	if (position[1] > 0+lines) {
		position[1] -= lines-1;
	}
	for (int i = 0; i < terminal_width*(terminal_height-lines); i++) {
		terminal_data[i].character = terminal_data[i+terminal_width*lines].character;
		terminal_data[i].fg_color = terminal_data[i+terminal_width*lines].fg_color;
		terminal_data[i].bg_color = terminal_data[i+terminal_width*lines].bg_color;
	}
	for (int i = terminal_width*(terminal_height-lines); i < terminal_width*terminal_height; i++) {
		terminal_data[i].character = 0x00;
		terminal_data[i].fg_color = 0x00;
		terminal_data[i].bg_color = 0x00;
	}
	rerender();
}

void set_cursor_position(int x, int y) {
	position[0] = x;
	position[1] = y;
}

int* get_cursor_position() {
	return position;
}

void clear_buffer() {
	draw_rectangle(0, 0, terminal_width*CHAR_WIDTH, terminal_height*CHAR_HEIGHT, 0, 0, 0);
	for (int i = 0; i < terminal_width*terminal_height; i++) {
		terminal_data[i].character = 0x00;
		terminal_data[i].fg_color = 0x00;
		terminal_data[i].bg_color = 0x00;
	}
	set_cursor_position(0, 1);
}

void print(const char *s) {
	int x = position[0];
	int y = position[1];

	for (int i = 0; i < strlen(s); i++) {
		if (s[i] == '\n') {
			if (y == terminal_height-1) {
				scroll(terminal_width/5);
				x = 0;
				y = position[1];
				continue;
			}
			x = 0;
			y++;
			continue;
		}
		putc(x, y, s[i], dst.fg, dst.bg);
		x += 1;
	}

	position[0] = x;
	position[1] = y;
}

int load_font(char *s, efi_physical_address_t framebuffer_base, uint32_t pixels_per_scanline, uint32_t width, uint32_t height) {
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

	dst.ptr = (unsigned char*)framebuffer_base;
	dst.w = width;
	dst.h = height;
	dst.p = sizeof(unsigned int) * pixels_per_scanline;
	dst.fg = 0xFFFFFFFF;

	ssfn_select(&ctx, SSFN_FAMILY_ANY, NULL, SSFN_STYLE_REGULAR | SSFN_STYLE_NOCACHE, 20);
	position[1] = ctx.size;

	terminal_width = width/CHAR_WIDTH;
	terminal_height = height/CHAR_HEIGHT;
	return 0;
}

void init_buffer() {
	terminal_data = malloc(sizeof(text_entry_t)*terminal_width*terminal_height);
}

void free_font() {
    ssfn_free(&ctx);
    free(font);
	free(terminal_data);
}

ssfn_t* get_ctx() {
	return &ctx;
}

void clear_char(int x, int y) {
	int real_x = x*CHAR_WIDTH;
	int real_y = (y-1)*CHAR_HEIGHT;
	draw_rectangle(real_x+5, real_y+3, CHAR_WIDTH, CHAR_HEIGHT, 0, 0, 0);

	terminal_data[y*terminal_width+x].character = 0x00;
	terminal_data[y*terminal_width+x].fg_color = 0x00;
	terminal_data[y*terminal_width+x].bg_color = 0x00;
	// putc(x, y, '█', 0xFF000000, 0xFF000000);
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

void err_prefix() {
	printf("[");
	set_fg(0xFFFF0000);
	printf("ERR ");
	set_fg(0xFFFFFFFF);
	printf("] ");
}
