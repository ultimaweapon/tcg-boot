#include "../../linux.h"

#include "../../config.h"

#include <efi.h>
#include <efilib.h>

#include <asm/bootparam.h>

void
linux_boot(unsigned char *kernel, unsigned char *initrd)
{
	const struct config *conf = config_get();
	struct boot_params *params;
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

	// clean up
	FreePool(params);
}
