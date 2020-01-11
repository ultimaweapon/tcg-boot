#include "main.h"

#include "config.h"
#include "path.h"

#include <efi.h>
#include <efilib.h>

EFI_HANDLE tcg;

static unsigned char * load_linux(UINTN *pages)
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
	*pages = info->FileSize / 4096 + 1;
	es = BS->AllocatePages(AllocateAnyPages, EfiLoaderCode, *pages, &addr);

	if (EFI_ERROR(es)) {
		Print(L"Failed to allocate %u pages: %r\n", *pages, es);
		goto end_with_info;
	}

	size = *pages * 4096;
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
	return (unsigned char *)addr;
}

static void * prepare_linux(UINTN *pages)
{
	unsigned char *kernel;

	// first, load linux into memory
	kernel = load_linux(pages);

	if (!kernel) {
		return NULL;
	}

	return kernel;
}

EFI_STATUS efi_main(EFI_HANDLE img, EFI_SYSTEM_TABLE *st)
{
	void *kernel;
	UINTN pages;

	// initialize libraries
	InitializeLib(img, st);

	// initialize sub-modules
	tcg = img;

	if (!config_init()) {
		goto fail;
	}

	// boot linux
	kernel = prepare_linux(&pages);

	if (!kernel) {
		goto fail_with_config;
	}

	BS->FreePages((EFI_PHYSICAL_ADDRESS)kernel, pages);

	// clean up
	config_term();

	return EFI_SUCCESS;

fail_with_config:
	config_term();

fail:
	Print(L"Press any key to exit\n");
	Pause();

	return EFI_ABORTED;
}
