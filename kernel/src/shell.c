#include <uefi.h>
#include "fonts.h"
#include <stdbool.h>
#include "string.h"
#include "mouse.h"
#include "graphics.h"

int rmtree(char* path) {
	DIR *d;
	struct dirent *de;
	if ((d = opendir(path))) {
		while ((de = readdir(d)) != NULL) {
			if (strcmp(de->d_name, "..") == 0 || strcmp(de->d_name, ".") == 0) {
				continue;
			}
			char s[262];
			strcpy(s, path);
			if (strlen(s) != 1) {
				strcat(s, "\\");
			}
			strcat(s, de->d_name);
			if (de->d_type == DT_DIR) {
				if (rmtree(s) != 0) {
					return -1;
				}
			} else {
				if (remove(s) != 0) {
					return -1;
				}
			}
		}
	} else {
		return -1;
	}
	return rmdir((wchar_t*)path);
}

int shell(int screen_width, int screen_height) {

	int* position = malloc(sizeof(int)*2);

	efi_status_t status;
	efi_input_key_t key;

	set_fg(0xFF00FFFF);
	print("LandOS\n");
	set_fg(0xFFFFFFFF);

	position = get_cursor_position();

	char* current_path = malloc(sizeof(char)*262);
	current_path[0] = '\\';
	current_path[1] = '\0';

	while (true) {

		set_fg(0xFF00FFFF);
		print(current_path);
		set_fg(0xFFFFFFFF);
		print("> _");

		set_cursor_position(position[0]-1, position[1]);

		char* cmd = malloc(sizeof(char)*128);
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

			usleep(10);

		}

		if (cmd_len == 0) {
			free(cmd);
			continue;
		}

		char* cmd_cloned = malloc(sizeof(char)*128);
		strcpy(cmd_cloned, cmd);

		char* token = malloc(sizeof(char)*64);
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
		} else if (strcmp(token, "getkey") == 0) {
			print("waiting for key press...\n");
			while (true) {
				status = ST->ConIn->ReadKeyStroke(ST->ConIn, &key);
				if (key.ScanCode != 0x00 || key.UnicodeChar != 0x00) {
					char s[4];
					print("Scan code: 0x");
					sprintf(s, "%2x", key.ScanCode);
					print(s);
					print("\nUnicode character: ");
					sprintf(s, "%c", key.UnicodeChar);
					print(s);
					print("\n");
					break;
				}
			}
		} else if (strcmp(token, "ls") == 0) {
			DIR *d;
			struct dirent *de;
			if ((d = opendir(current_path))) {
				while ((de = readdir(d)) != NULL) {
					print(de->d_name);
					print("\n");
				}
			} else {
				print("unable to open directory\n");
			}
		} else if (strcmp(token, "cd") == 0) {
			token = strtok(NULL, " ");
			if (token != NULL) {
				if (strcmp(token, "..") == 0) {
					char parent_dir[262];
					parent_dir[0] = '\\';
					parent_dir[1] = '\0';

					char current_path_cloned[262];
					strcpy(current_path_cloned, current_path);
					char* token_dir = strtok(current_path_cloned, "\\");
					while (token_dir != NULL) {
						strcpy(parent_dir, token_dir);
						token_dir = strtok(NULL, "\\");
					}
					int l = strlen(current_path);
					current_path[strlen(current_path)-strlen(parent_dir)] = '\0';
					if (strlen(current_path) != 1) {
						current_path[strlen(current_path)-1] = '\0';
					}
					// printf("new: %s, %s, %d, %d, %d\n", current_path, parent_dir, l, strlen(parent_dir), l-strlen(parent_dir));
				} else if (strcmp(token, ".") != 0) {
					DIR *d;
					struct dirent *de;
					if ((d = opendir(current_path))) {
						bool exists = false;
						while ((de = readdir(d)) != NULL) {
							if (strcmp(de->d_name, token) == 0) {
								if (current_path[strlen(current_path)-1] != '\\') {
									strcat(current_path, "\\");
								}
								strcat(current_path, de->d_name);
								exists = true;
								break;
							}
						}
						if (!exists) {
							print("invalid directory\n");
						}
					} else {
						print("unable to open directory\n");
					}
				}
			}
		} else if (strcmp(token, "clear") == 0) {
			clear_buffer();
		} else if (strcmp(token, "cat") == 0) {
			token = strtok(NULL, " ");
			if (token == NULL) {
				print("invalid usage\ncat (file)\n");
			} else {
				char *path = malloc(sizeof(char)*262);
				strcpy(path, current_path);
				if (strlen(path) != 1) {
					strcat(path, "\\");
				}
				strcat(path, token);
				FILE *f;
				if ((f = fopen(path, "r"))) {
					fseek(f, 0, SEEK_END);
					int size = ftell(f);
					fseek(f, 0, SEEK_SET);
					if (size > 1024) {
						print("file too big\n");
					} else {
						char *buffer = malloc(1025);

						if (!buffer) {
							print("unable to allocate memory for file data\n");
						} else {
							fread(buffer, size, 1, f);
							buffer[size] = '\0';
							print(buffer);
							print("\n");
						}

						free(buffer);
					}
				} else {
					print("could not open file\n");
				}
				free(path);
				fclose(f);
			}
		} else if (strcmp(token, "mkdir") == 0) {
			token = strtok(NULL, " ");
			if (token == NULL) {
				print("invalid usage\nmkdir (directory)\n");
			} else {
				char *path = malloc(sizeof(char)*262);
				strcpy(path, current_path);
				if (strlen(path) != 1) {
					strcat(path, "\\");
				}
				strcat(path, token);

				if (mkdir(path, 0) != 0) {
					print("could not create directory\n");
				}

				free(path);
			}
		} else if (strcmp(token, "rm") == 0) {
			token = strtok(NULL, " ");
			if (token == NULL) {
				print("invalid usage\nrm (file)\n");
			} else {
				char *path = malloc(sizeof(char)*262);
				strcpy(path, current_path);
				if (strlen(path) != 1) {
					strcat(path, "\\");
				}
				strcat(path, token);

				if (remove(path) != 0) {
					print("could not delete file\n");
				}

				free(path);
			}
		} else if (strcmp(token, "rmdir") == 0) {
			token = strtok(NULL, " ");
			if (token == NULL) {
				print("invalid usage\nrmdir (directory)\n");
			} else {
				char *path = malloc(sizeof(char)*262);
				strcpy(path, current_path);
				if (strlen(path) != 1) {
					strcat(path, "\\");
				}
				strcat(path, token);

				if (rmtree(path) != 0) {
					print("could not delete directory\n");
				}

				free(path);
			}
		} else if (strcmp(token, "touch") == 0) {
			token = strtok(NULL, " ");
			if (token == NULL) {
				print("invalid usage\ntouch (file)\n");
			} else {
				char *path = malloc(sizeof(char)*262);
				strcpy(path, current_path);
				if (strlen(path) != 1) {
					strcat(path, "\\");
				}
				strcat(path, token);

				FILE *f = fopen(path, "w");
				if (!f) {
					print("could not create file\n");
				} else {
					fclose(f);
				}

				free(path);
			}
		} else if (strcmp(token, "help") == 0) {
			print("help                shows this menu\n");
			print("shutdown            shuts down the system\n");
			print("reboot (cold/warm)  reboots the system, cold reboot by default\n");
			print("getkey              get info about a keypress (for debugging)\n");
			print("ls                  lists current directory\n");
			print("cd (directory)      change current directory\n");
			print("clear               clear the screen\n");
			print("cat (file)          outputs contents of file\n");
			print("mkdir (directory)   create a directory\n");
			print("rm (file)           deletes a file\n");
			print("rmdir (directory)   deletes a directory recursively\n");
			print("touch (file)        creates a file\n");
		}

		free(token);
		free(cmd);
		free(cmd_cloned);

	}

	free(current_path);

	return 0;

}