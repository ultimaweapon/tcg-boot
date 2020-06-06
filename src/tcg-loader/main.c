#include "main.h"

#include "config.h"
#include "path.h"

#include <asm/bootparam.h>

#include <efi.h>
#include <efilib.h>

struct allocated_pages {
	UINTN count;
	unsigned char *start;
};

EFI_HANDLE tcg;

static
void
free_pages(struct allocated_pages *p)
{
	EFI_STATUS es;

	if (!p->count) {
		return;
	}

	es = BS->FreePages((EFI_PHYSICAL_ADDRESS)p->start, p->count);

	if (EFI_ERROR(es)) {
		Print(
			L"Failed to free %u pages starting at %lX: %r\n",
			p->count,
			p->start,
			es);
		return;
	}

	ZeroMem(p, sizeof(*p));
}

static
bool
load_linux(struct allocated_pages *res)
{
	const struct config *conf = config_get();
	bool success = false;
	EFI_DEVICE_PATH_PROTOCOL *path;
	EFI_FILE_PROTOCOL *root, *file;
	EFI_STATUS es;
	EFI_FILE_INFO *info;
	EFI_PHYSICAL_ADDRESS addr;
	UINTN size;

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
	res->count = EFI_SIZE_TO_PAGES(info->FileSize);
	addr = 0xFFFFFFFF;

	es = BS->AllocatePages(
		AllocateMaxAddress,
		EfiLoaderData,
		res->count,
		&addr);

	if (EFI_ERROR(es)) {
		Print(
			L"Failed to allocate %u pages for %D: %r\n",
			res->count,
			conf->kernel,
			es);
		goto end_with_info;
	}

	res->start = (unsigned char *)addr;
	size = info->FileSize;
	es = file->Read(file, &size, res->start);

	if (EFI_ERROR(es)) {
		Print(L"Failed to read %D: %r\n", conf->kernel, es);
		free_pages(res);
		goto end_with_info;
	}

	success = true;

	// clean up
end_with_info:
	FreePool(info);

end_with_file:
	file->Close(file);

end_with_root:
	root->Close(root);

end:
	return success;
}

static
void
boot_linux(void)
{
	const struct config *conf = config_get();
	struct allocated_pages kernel;
	struct boot_params *params;
	EFI_STATUS es;

	// load kernel into memory
	if (!load_linux(&kernel)) {
		return;
	}

	if (CompareMem(kernel.start + 0x202, "HdrS", 4)) {
		Print(L"%D is not a Linux kernel or it is too old\n", conf->kernel);
		goto fail_with_kernel;
	}

	// prepare parameters
	params = AllocateZeroPool(sizeof(*params));

	if (!params) {
		Print(
			L"Failed to allocate %u bytes for boot parameters\n",
			sizeof(*params));
		goto fail_with_kernel;
	}

	CopyMem(
		&params->hdr,
		kernel.start + 0x01F1,
		(0x0202 + kernel.start[0x0201]) - 0x01F1);

	// clean up
	FreePool(params);

fail_with_kernel:
	free_pages(&kernel);
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
