#include <uefi.h>
#include <stdbool.h>
#include "stdlib.c"
#include "time.c"
#include "kernel.h"

efi_gop_t *gop = NULL;
efi_gop_mode_info_t *gop_info = NULL;

#define white 255, 255, 255
#define black 0, 0, 0
#define ms * 1000

#define ELFMAG      "\177ELF"
#define SELFMAG     4
#define EI_CLASS    4       // File class byte index
#define ELFCLASS64  2       // 64-bit objects
#define EI_DATA     5       // Data encoding byte index
#define ELFDATA2LSB 1       // 2's complement, little endian
#define ET_EXEC     2       // Executable file
#define PT_LOAD     1       // Loadable program segment
#define EM_MACH     62      // AMD x86-64 architecture

typedef struct {
    uint8_t  e_ident[16];   // Magic number and other info
    uint16_t e_type;        // Object file type
    uint16_t e_machine;     // Architecture
    uint32_t e_version;     // Object file version
    uint64_t e_entry;       // Entry point virtual address
    uint64_t e_phoff;       // Program header table file offset
    uint64_t e_shoff;       // Section header table file offset
    uint32_t e_flags;       // Processor-specific flags
    uint16_t e_ehsize;      // ELF header size in bytes
    uint16_t e_phentsize;   // Program header table entry size
    uint16_t e_phnum;       // Program header table entry count
    uint16_t e_shentsize;   // Section header table entry size
    uint16_t e_shnum;       // Section header table entry count
    uint16_t e_shstrndx;    // Section header string table index
} elf64_ehdr;

typedef struct {
    uint32_t p_type;        // Segment type
    uint32_t p_flags;       // Segment flags
    uint64_t p_offset;      // Segment file offset
    uint64_t p_vaddr;       // Segment virtual address
    uint64_t p_paddr;       // Segment physical address
    uint64_t p_filesz;      // Segment size in file
    uint64_t p_memsz;       // Segment size in memory
    uint64_t p_align;       // Segment alignment
} elf64_phdr;

static inline void info_prefix() {
	printf("[");
	ST->ConOut->SetAttribute(ST->ConOut, EFI_GREEN);
	printf("INFO");
	ST->ConOut->SetAttribute(ST->ConOut, EFI_LIGHTGRAY);
	printf("] ");
}

static inline void err_prefix() {
	printf("[");
	ST->ConOut->SetAttribute(ST->ConOut, EFI_RED);
	printf("ERR ");
	ST->ConOut->SetAttribute(ST->ConOut, EFI_LIGHTGRAY);
	printf("] ");
}

void poll() {
	while (true) {
		sleep(1);
	};
}

uintptr_t get_kernel_entry() {
	FILE *f;
	char *buff;
	long int size;

	if ((f = fopen("\\kernel\\kernel.elf", "r"))) {
		fseek(f, 0, SEEK_END);
		size = ftell(f);
		fseek(f, 0, SEEK_SET);
		buff = malloc(size+1);
		if (!buff) {
			err_prefix();
			printf("unable to allocate memory for kernel\n");
			poll();
		}
		fread(buff, size, 1, f);
		fclose(f);
	} else {
		err_prefix();
		printf("unable to open kernel elf\n");
		poll();
	}

	elf64_ehdr *elf = (elf64_ehdr*)buff;

	info_prefix();
	printf("checking kernel magic number\n");
	if (memcmp(elf->e_ident, ELFMAG, SELFMAG)) {
		err_prefix();
		printf("magic number does not match\n");
		poll();
	}

	info_prefix();
	printf("checking kernel architecture\n");
	if (elf->e_ident[EI_CLASS] != ELFCLASS64) {
		err_prefix();
		printf("invalid architecture\n");
		poll();
	}

	info_prefix();
	printf("checking kernel elfdata\n");
	if (elf->e_ident[EI_DATA] != ELFDATA2LSB) {
		err_prefix();
		printf("invalid elfdata2lsb\n");
		poll();
	}

	info_prefix();
	printf("checking if kernel is an executable\n");
	if (elf->e_type != ET_EXEC) {
		err_prefix();
		printf("not an executable\n");
		poll();
	}

	info_prefix();
	printf("checking machine architecture\n");
	if (elf->e_machine != EM_MACH) {
		err_prefix();
		printf("invalid architecture match\n");
		poll();
	}

	info_prefix();
	printf("checking headers\n");
	if (!(elf->e_phnum > 0)) {
		err_prefix();
		printf("no program headers\n");
		poll();
	}

	info_prefix();
	printf("loading kernel segments\n");
	elf64_phdr *phdr = (elf64_phdr*)(buff+elf->e_phoff);
	for (int i = 0; i < elf->e_phnum; i++) {
		if (phdr->p_type == PT_LOAD) {
			info_prefix();
			printf("ELF segment %d bytes\n", phdr->p_filesz);
			memcpy((void*)phdr->p_vaddr, buff + phdr->p_offset, phdr->p_filesz);
			memset((void*)(phdr->p_vaddr + phdr->p_filesz), 0, phdr->p_memsz - phdr->p_filesz);
		}

		phdr = (elf64_phdr*)((uint8_t*)phdr+elf->e_phentsize);
	}

	uintptr_t entry = elf->e_entry;

	free(buff);

	return entry;

}

int main(int argc, char **argv) {
	efi_status_t status;
	efi_guid_t gop_guid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
	uintn_t isiz = sizeof(efi_gop_mode_info_t);

	ST->ConOut->ClearScreen(ST->ConOut);

	info_prefix();
	printf("setting random seed\n");

	struct tm* t = localtime(NULL);
	srand(t->tm_sec*t->tm_hour*t->tm_min*t->tm_year);

	info_prefix();
	printf("getting gop\n");

	status = BS->LocateProtocol(&gop_guid, NULL, (void**)&gop);
	if (EFI_ERROR(status) || !gop) {
		err_prefix();
		printf("unable to get graphics output protocol\n");
		poll();
	}

	info_prefix();
	printf("getting current video mode\n");

	status = gop->QueryMode(gop, gop->Mode ? gop->Mode->Mode : 0, &isiz, &gop_info);
	if(status == EFI_NOT_STARTED || !gop->Mode) {
		status = gop->SetMode(gop, 0);
	}
	if(EFI_ERROR(status)) {
		err_prefix();
		printf("unable to get current video mode\n");
		poll();
	}

	ST->ConOut->ClearScreen(ST->ConOut);
	int selected = -1;
	for (int i = 0; i < gop->Mode->MaxMode; i++) {
		status = gop->QueryMode(gop, i, &isiz, &gop_info);
		if (EFI_ERROR(status)) {
			continue;
		}
		if (gop_info->HorizontalResolution == 1920 && gop_info->VerticalResolution == 1080) {
			selected = i;
		}
		if (i == selected) {
			ST->ConOut->SetAttribute(ST->ConOut, EFI_BACKGROUND_LIGHTGRAY | EFI_BLACK);
		}
		printf("  %4d   %4d\n", gop_info->HorizontalResolution, gop_info->VerticalResolution);
		ST->ConOut->SetAttribute(ST->ConOut, EFI_BACKGROUND_BLACK | EFI_LIGHTGRAY);
	}

	if (selected == -1) {
		selected = 0;
	}

	ST->ConOut->SetCursorPosition(ST->ConOut, 0, selected);
	gop->QueryMode(gop, selected, &isiz, &gop_info);
	ST->ConOut->SetAttribute(ST->ConOut, EFI_BACKGROUND_LIGHTGRAY | EFI_BLACK);
	printf("  %4d   %4d\n", gop_info->HorizontalResolution, gop_info->VerticalResolution);
	ST->ConOut->SetAttribute(ST->ConOut, EFI_BACKGROUND_BLACK | EFI_LIGHTGRAY);

	efi_input_key_t key;
	while (true) {
		status = ST->ConIn->ReadKeyStroke(ST->ConIn, &key);
		if (key.ScanCode == 0x2) {

			if (selected == min(29, gop->Mode->MaxMode)) {
				continue;
			}

			ST->ConOut->SetCursorPosition(ST->ConOut, 0, selected);
			gop->QueryMode(gop, selected, &isiz, &gop_info);
			printf("  %4d   %4d\n", gop_info->HorizontalResolution, gop_info->VerticalResolution);

			ST->ConOut->SetCursorPosition(ST->ConOut, 0, selected+1);
			gop->QueryMode(gop, selected+1, &isiz, &gop_info);
			ST->ConOut->SetAttribute(ST->ConOut, EFI_BACKGROUND_LIGHTGRAY | EFI_BLACK);
			printf("  %4d   %4d\n", gop_info->HorizontalResolution, gop_info->VerticalResolution);
			ST->ConOut->SetAttribute(ST->ConOut, EFI_BACKGROUND_BLACK | EFI_LIGHTGRAY);

			selected++;
		} else if (key.ScanCode == 0x1) {

			if (selected == 0) {
				continue;
			}

			ST->ConOut->SetCursorPosition(ST->ConOut, 0, selected);
			gop->QueryMode(gop, selected, &isiz, &gop_info);
			printf("  %4d   %4d\n", gop_info->HorizontalResolution, gop_info->VerticalResolution);

			ST->ConOut->SetCursorPosition(ST->ConOut, 0, selected-1);
			gop->QueryMode(gop, selected-1, &isiz, &gop_info);
			ST->ConOut->SetAttribute(ST->ConOut, EFI_BACKGROUND_LIGHTGRAY | EFI_BLACK);
			printf("  %4d   %4d\n", gop_info->HorizontalResolution, gop_info->VerticalResolution);
			ST->ConOut->SetAttribute(ST->ConOut, EFI_BACKGROUND_BLACK | EFI_LIGHTGRAY);

			selected--;
		} else {
			status = ST->ConIn->ReadKeyStroke(ST->ConIn, &key);
			if (key.UnicodeChar == 0x0D) {
				break;
			} else {
				continue;
			}
		}
		usleep(100);
	}

	ST->ConOut->ClearScreen(ST->ConOut);

	info_prefix();
	printf("setting video mode\n");

	status = gop->SetMode(gop, selected);
	if (EFI_ERROR(status)) {
		err_prefix();
		printf("unable to set video mode\n");
		poll();
	}

	status = gop->QueryMode(gop, gop->Mode->Mode, &isiz, &gop_info);
	info_prefix();
	printf("Currently loaded video mode: %d, %d\n", gop_info->HorizontalResolution, gop_info->VerticalResolution);

	// info_prefix();
	// printf("loading font\n");

	// int e;
	// if ((e = load_font("\\fonts\\MesloLGS NF.sfn", gop)) != 0) {
	// 	if (e == -1) {
	// 		err_prefix();
	// 		printf("could not open font\n");
	// 	} else if (e == -2) {
	// 		err_prefix();
	// 		printf("unable to allocate memory for font\n");
	// 	}
	// 	poll();
	// }

	// info_prefix();
	// print("allocating screen buffer\n");
	// init_buffer();
	// info_prefix();
	// print("init graphics\n");

	// init_graphics(gop->Mode->FrameBufferBase, gop->Mode->Information->PixelsPerScanLine, gop->Mode->Information->HorizontalResolution, gop->Mode->Information->HorizontalResolution);
	// usleep(500 ms);
	// clear_buffer();

	// set_cursor_position(0, 1);
	// int code = shell();
	// usleep(250 ms);

	// info_prefix();
	// print("unallocating screen data\n");
	// usleep(500 ms);
	// free_font();

	// info_prefix();
	// printf("executing instruction\n");
	// usleep(500 ms);

	// if (code == 1) {
	// 	ST->RuntimeServices->ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0, 0);
	// } else if (code == 2) {
	// 	ST->RuntimeServices->ResetSystem(EfiResetCold, EFI_SUCCESS, 0, 0);
	// } else if (code == 3) {
	// 	ST->RuntimeServices->ResetSystem(EfiResetWarm, EFI_SUCCESS, 0, 0);
	// }



	// uintptr_t entry = get_kernel_entry();
	// int (*kernel_main)(graphics_information*) = ((__attribute__((sysv_abi)) int (*)(graphics_information*) ) entry);
	graphics_information g;
	g.framebuffer_base = gop->Mode->FrameBufferBase;
	g.width = gop->Mode->Information->VerticalResolution;
	g.height = gop->Mode->Information->HorizontalResolution;
	g.pixels_per_scanline = gop->Mode->Information->PixelsPerScanLine;
	// int return_code = kernel_main(&g);
	// printf("kernel returned %d\n", return_code);

	kernel_main(&g);

	poll();

	return 0;
}
