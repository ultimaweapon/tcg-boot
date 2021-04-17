#include "../../linux.h"

#include "../../config.h"
#include "../../main.h"

#include <efi.h>
#include <efilib.h>

#include <asm/bootparam.h>

#include <stdint.h>

struct gdt {
	uint64_t padding: 48;
	uint64_t limit: 16;
	uint64_t *base;
};

__attribute__((noreturn))
static
void
start_kernel(
	uint32_t kernel,
	const struct boot_params *params,
	const struct gdt *gdt)
{
	// setup segment registers
	asm ("mov %0, %%rsi" : : "m" (params));
	asm ("mov %0, %%rbx" : : "m" (kernel));
	asm ("mov %0, %%rax" : : "m" (gdt));
	asm ("lgdt 6(%rax)");
	asm ("mov $0x18, %rax");
	asm ("mov %ax, %ds");
	asm ("mov %ax, %es");
	asm ("mov %ax, %fs");
	asm ("mov %ax, %gs");
	asm ("mov %ax, %ss");

	// jmp into the kernel
	asm ("add $0x200, %rbx");
	asm ("pushq $0x10");
	asm ("pushq %rbx");
	asm ("lretq");

	__builtin_unreachable();
}

void
linux_boot(unsigned char *kernel, unsigned char *initrd, size_t rdsize)
{
	const struct config *conf = config_get();
	struct boot_params *params;
	struct gdt *gdt;
	EFI_MEMORY_DESCRIPTOR *map;
	UINTN count, key, size;
	UINT32 ver;
	EFI_STATUS es;

	// prepare parameters
	params = AllocateZeroPool(sizeof(*params));

	if (!params) {
		Print(
			L"Failed to allocate %u bytes for boot parameters\n",
			sizeof(*params));
		return;
	}

	CopyMem(&params->hdr, kernel + 0x01F1, (0x0202 + kernel[0x0201]) - 0x01F1);

	params->hdr.vid_mode = 0xFFFF; // normal
	params->hdr.type_of_loader = 0xFF;
	params->hdr.loadflags = QUIET_FLAG;
	params->hdr.ramdisk_image = (intptr_t)initrd;
	params->hdr.ramdisk_size = rdsize;

	if (params->hdr.version >= 0x0202) {
		// FIXME: ensure command_line is not above 4gb
		// FIXME: use "auto" in case of no command line in the configuration
		params->hdr.cmd_line_ptr = (intptr_t)conf->command_line;
	}

	// setup gdt
	gdt = AllocateZeroPool(sizeof(*gdt));

	if (!gdt) {
		Print(L"Failed to allocated %u bytes for GDT\n", sizeof(*gdt));
		goto fail_with_params;
	}

	gdt->limit = 8 * 4;
	gdt->base = AllocateZeroPool(gdt->limit);

	if (!gdt->base) {
		Print(L"Failed to allocated %u bytes for GDT entries\n", gdt->limit);
		goto fail_with_gdt;
	}

	gdt->base[2] = 0x00AF9A000000FFFF; // cs
	gdt->base[3] = 0x00CF92000000FFFF; // ds
	gdt->limit--;

	// terminate boot service
	map = LibMemoryMap(&count, &key, &size, &ver);

	if (!map) {
		Print(L"Failed to get system memory map\n");
		goto fail_with_gdt;
	}

	es = BS->ExitBootServices(tcg, key);

	if (EFI_ERROR(es)) {
		Print(L"Failed to terminate boot services: %r\n", es);
		goto fail_with_map;
	}

	// jump to the kernel
	start_kernel((intptr_t)kernel, params, gdt);

	// clean up
fail_with_map:
	FreePool(map);

fail_with_gdt:
	FreePool(gdt->base);
	FreePool(gdt);

fail_with_params:
	FreePool(params);
}
