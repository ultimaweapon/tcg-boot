#include "main.h"

#include "config.h"
#include "path.h"

#include <asm/bootparam.h>

#include <efi.h>
#include <efilib.h>

EFI_HANDLE tcg;

static
EFI_PHYSICAL_ADDRESS
load_linux(UINTN *pages)
{
	const struct config *conf = config_get();
	EFI_PHYSICAL_ADDRESS addr;
	EFI_DEVICE_PATH_PROTOCOL *path;
	EFI_FILE_PROTOCOL *root, *file;
	EFI_STATUS es;
	EFI_FILE_INFO *info;
	UINTN size;

	addr = 0;

	// open volume that contain kernel
	path = conf->kernel;
	root = path_open_device_volume(&path);

	if (!root) {
		Print(L"Failed to open volume of %D\n", conf->kernel);
		goto end;
	}

	// open kernel
	es = root->Open(
		root,
		&file,
		path_get_file_path(path),
		EFI_FILE_MODE_READ,
		0);

	if (EFI_ERROR(es)) {
		Print(L"Failed to open %D: %r\n", conf->kernel, es);
		goto end_with_root;
	}

	info = LibFileInfo(file);

	if (!info) {
		Print(L"Failed to get file information for %D\n", conf->kernel);
		goto end_with_file;
	}

	// load kernel
	*pages = EFI_SIZE_TO_PAGES(info->FileSize);

	es = BS->AllocatePages(
		AllocateAnyPages,
		EfiLoaderData,
		*pages,
		&addr);

	if (EFI_ERROR(es)) {
		Print(
			L"Failed to allocate %u pages for %D: %r\n",
			*pages,
			conf->kernel,
			es);
		addr = 0;
		goto end_with_info;
	}

	size = info->FileSize;
	es = file->Read(file, &size, (VOID *)addr);

	if (EFI_ERROR(es)) {
		Print(L"Failed to read %D: %r\n", conf->kernel, es);

		es = BS->FreePages(addr, *pages);

		if (EFI_ERROR(es)) {
			Print(
				L"Failed to free %u pages starting at %lX: %r\n",
				*pages,
				addr,
				es);
		}

		addr = 0;
	}

	// clean up
end_with_info:
	FreePool(info);

end_with_file:
	file->Close(file);

end_with_root:
	root->Close(root);

end:
	return addr;
}

static
void
boot_linux(void)
{
	const struct config *conf = config_get();
	unsigned char *kernel;
	UINTN pages, nheader;
	struct boot_params *params;
	EFI_STATUS es;

	// load kernel into memory
	kernel = (unsigned char *)load_linux(&pages);

	if (!kernel) {
		return;
	}

	if (CompareMem(kernel + 0x202, "HdrS", 4)) {
		Print(L"%D is not a Linux kernel or it is too old\n", conf->kernel);
		goto fail_with_kernel;
	}

	// prepare parameters
	params = AllocateZeroPool(sizeof(struct boot_params));

	if (!params) {
		Print(
			L"Failed to allocate %u bytes for boot parameters\n",
			sizeof(struct boot_params));
		goto fail_with_kernel;
	}

	nheader = (0x0202 + kernel[0x0201]) - 0x01F1;

	CopyMem(&params->hdr, kernel + 0x01F1, nheader);

	// clean up
	FreePool(params);

fail_with_kernel:
	es = BS->FreePages((EFI_PHYSICAL_ADDRESS)kernel, pages);

	if (EFI_ERROR(es)) {
		Print(
			L"Failed to free %u pages starting at %lX: %r\n",
			pages,
			kernel,
			es);
	}
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
