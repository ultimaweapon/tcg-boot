#include "main.h"

#include "config.h"
#include "linux.h"
#include "page.h"

#include <efi.h>
#include <efilib.h>

EFI_HANDLE tcg;

static
void
boot_linux(void)
{
	struct page_alloc kernel, initrd;
	size_t rdsize;

	// load kernel and initrd
	if (!linux_kernel_load(&kernel)) {
		return;
	}

	if (!linux_initrd_load(&initrd, &rdsize)) {
		goto fail_with_kernel;
	}

	// boot
	linux_boot(kernel.addr, initrd.addr, rdsize);

	// clean up
	page_free(&initrd);

fail_with_kernel:
	page_free(&kernel);
}

EFI_STATUS
efi_main(EFI_HANDLE img, EFI_SYSTEM_TABLE *st)
{
	// initialize libraries
	InitializeLib(img, st);

	// initialize sub-modules
	tcg = img;

	if (!config_init()) {
		goto fail;
	}

	// boot linux
	boot_linux();

	// clean up
	config_term();

fail:
	Print(L"Press any key to exit\n");
	Pause();

	return EFI_ABORTED;
}
