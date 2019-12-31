#include "main.h"

#include "config.h"

#include <efi.h>
#include <efilib.h>

EFI_HANDLE tcg;

EFI_STATUS efi_main(EFI_HANDLE img, EFI_SYSTEM_TABLE *st)
{
	// initialize libraries
	InitializeLib(img, st);

	// initialize sub-modules
	tcg = img;

	if (!config_init()) {
		return EFI_ABORTED;
	}

	// clean up
	config_term();

	return EFI_SUCCESS;
}
