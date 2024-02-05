#include <uefi.h>
#include "fonts.h"
#include <stdbool.h>
#include "graphics.h"

int shell() {

	int position[2] = { 0, 0 };

	efi_status_t status;
	efi_input_key_t key;

	set_fg(0xFF00FFFF);
	print("LandOS\n");
	set_fg(0xFFFFFFFF);

	while (true) {

		print("> _");

		get_cursor_position(position);
		set_cursor_position(position[0]-char_width(), position[1]);
		get_cursor_position(position);

		char* cmd = malloc(128);
		int cmd_len = 0;
		while (true) {
			status = ST->ConIn->ReadKeyStroke(ST->ConIn, &key);

			if (key.UnicodeChar != 0 && key.ScanCode == 0 && key.UnicodeChar != 0x0D && key.UnicodeChar != 0x08) {
				clear_char(position[0], position[1]);
				char s[2] = { key.UnicodeChar, '\0' };
				print(s);
				print("_");

				get_cursor_position(position);
				set_cursor_position(position[0]-char_width(), position[1]);
				get_cursor_position(position);
				cmd[cmd_len] = s[0];
				cmd_len++;
			} else if (key.UnicodeChar == 0x0D) {
				cmd[cmd_len] = '\0';
				clear_char(position[0], position[1]);
				print("\n");
				break;
			} else if (key.UnicodeChar == 0x08 && cmd_len > 0) {
				cmd_len--;
				clear_char(position[0], position[1]);
				set_cursor_position(position[0]-char_width(), position[1]);
				get_cursor_position(position);
				clear_char(position[0], position[1]);

				print("_");
				get_cursor_position(position);
				set_cursor_position(position[0]-char_width(), position[1]);
				get_cursor_position(position);
			}
		}

		if (cmd_len == 0) {
			free(cmd);
			continue;
		}

		char* cmd_cloned = malloc(128);
		strcpy(cmd_cloned, cmd);

		char* token = malloc(64);
		token = strtok(cmd, " ");
		if (strcmp(token, "shutdown") == 0) {
			set_cursor_position(0, get_ctx()->size);
			clear_screen();
			info_prefix();
			print("shutting down\n");
			return 1;
		} else if (strcmp(token, "reboot") == 0) {
			set_cursor_position(0, get_ctx()->size);
			clear_screen();
			info_prefix();
			print("rebooting\n");
			return 2;
		} else if (strcmp(token, "echo") == 0) {
			char s[2];
			for (int i = 5; i < strlen(cmd_cloned); i++) {
				s[0] = cmd_cloned[i];
				s[1] = '\0';
				print(s);
			}
			print("\n");
		} else if (strcmp(token, "help") == 0) {
			print("help shows this menu\n");
			print("shutdown   shuts down the system\n");
			print("reboot     reboots the system\n");
		}

		free(token);
		free(cmd);
		free(cmd_cloned);

	}

	return 0;

}