#include <uefi.h>
#include "fonts.h"
#include <stdbool.h>

efi_status_t status;
efi_guid_t ptr_guid = EFI_SIMPLE_POINTER_PROTOCOL_GUID;
efi_simple_pointer_protocol_t *ptr = NULL;
efi_simple_pointer_state_t state = {0};

int x = 0;
int y = 0;
bool has_mouse = true;

int get_mouse_position(int* pos, int width, int height) {
	if (!has_mouse) {
		return -2;
	}
	if (ptr == NULL) {
		status = ST->BootServices->LocateProtocol(&ptr_guid, NULL, (void**)&ptr);
		if (EFI_ERROR(status) || !ptr) {
			has_mouse = false;
			return -1;
		}
	}
	ptr->GetState(ptr, &state);
	x += state.RelativeMovementX/ptr->Mode->ResolutionX;
	y += state.RelativeMovementY/ptr->Mode->ResolutionY;
	if (x < 0) {
		x = 0;
	} else if (x > width) {
		x = width;
	}
	if (y < 0) {
		y = 0;
	} else if (y > height) {
		y = height;
	}
	pos[0] = x;
	pos[1] = y;
	return 0;
}
