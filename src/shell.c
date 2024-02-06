#include <uefi.h>
#include "fonts.h"
#include <stdbool.h>

int shell() {

	int* position = malloc(sizeof(int)*2);

	efi_status_t status;
	efi_input_key_t key;

	set_fg(0xFF00FFFF);
	print("LandOS\n");
	set_fg(0xFFFFFFFF);

	position = get_cursor_position();

	while (true) {

		print("> _");

		set_cursor_position(position[0]-1, position[1]);

		char* cmd = malloc(128);
		int cmd_len = 0;
		while (true) {
			status = ST->ConIn->ReadKeyStroke(ST->ConIn, &key);

			if (key.UnicodeChar != 0 && key.ScanCode == 0 && key.UnicodeChar != 0x0D && key.UnicodeChar != 0x08) {
				clear_char(position[0], position[1]);
				char s[2] = { key.UnicodeChar, '\0' };
				print(s);
				print("_");

				set_cursor_position(position[0]-1, position[1]);
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
				set_cursor_position(position[0]-1, position[1]);
				clear_char(position[0], position[1]);

				print("_");
				set_cursor_position(position[0]-1, position[1]);
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
			clear_buffer();
			info_prefix();
			print("shutting down\n");
			return 1;
		} else if (strcmp(token, "reboot") == 0) {
			clear_buffer();
			info_prefix();

			token = strtok(NULL, " ");
			if (token == NULL || strcmp(token, "cold") == 0) {
				print("cold rebooting\n");
				return 2;
			} else if (strcmp(token, "warm") == 0) {
				print("warm rebooting\n");
				return 3;
			} else {
				print("invalid argument\n");
			}
		} else if (strcmp(token, "echo") == 0) {
			char s[2];
			for (int i = 5; i < strlen(cmd_cloned); i++) {
				s[0] = cmd_cloned[i];
				s[1] = '\0';
				print(s);
			}
			print("\n");
		} else if (strcmp(token, "help") == 0) {
			print("help                   shows this menu\n");
			print("shutdown               shuts down the system\n");
			print("reboot (cold/warm)     reboots the system, cold reboot by default\n");
		}

		free(token);
		free(cmd);
		free(cmd_cloned);

	}

	return 0;

}