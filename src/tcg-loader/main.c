#include "main.h"

#include "config.h"
#include "path.h"

#include <asm/bootparam.h>

#include <efi.h>
#include <efilib.h>

#include <stddef.h>

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
bool
load_initrd(struct allocated_pages *res)
{
	const struct config *conf = config_get();
	bool success = false;
	UINTN size;
	struct opened_file {
		EFI_FILE_PROTOCOL *root, *file;
		EFI_FILE_INFO *info;
	} *files;
	EFI_STATUS es;
	EFI_PHYSICAL_ADDRESS addr;
	unsigned char *p;

	// prepare to load all initrd
	if (!conf->initrd.count) {
		ZeroMem(res, sizeof(*res));
		return true;
	}

	size = sizeof(*files) * conf->initrd.count;
	files = AllocateZeroPool(size);

	if (!files) {
		Print(L"Failed to allocated %u bytes for loading initrd\n", size);
		goto end;
	}

	// open all initrd and sum its size
	size = 0;

	for (size_t i = 0; i < conf->initrd.count; i++) {
		EFI_DEVICE_PATH_PROTOCOL *path = conf->initrd.paths[i];
		struct opened_file *file = &files[i];

		// open a volume that contain the image
		file->root = path_open_device_volume(&path);

		if (!file->root) {
			Print(L"Failed to open volume of %D\n", conf->initrd.paths[i]);
			goto end_with_files;
		}

		// open initrd
		es = file->root->Open(
			file->root,
			&file->file,
			path_get_file_path(path),
			EFI_FILE_MODE_READ,
			0);

		if (EFI_ERROR(es)) {
			Print(L"Failed to open %D: %r\n", conf->initrd.paths[i], es);
			goto end_with_files;
		}

		// get initrd info
		file->info = LibFileInfo(file->file);

		if (!file->info) {
			Print(
				L"Failed to get file information for %D\n",
				conf->initrd.paths[i]);
			goto end_with_files;
		}

		size += file->info->FileSize;
	}

	// allocate pages for all initrd
	if (!size) {
		ZeroMem(res, sizeof(*res));
		success = true;
		goto end_with_files;
	}

	res->count = EFI_SIZE_TO_PAGES(size);
	addr = 0xFFFFFFFF;

	es = BS->AllocatePages(
		AllocateMaxAddress,
		EfiLoaderData,
		res->count,
		&addr);

	if (EFI_ERROR(es)) {
		Print(
			L"Failed to allocate %u pages for all initrd: %r\n",
			res->count,
			es);
		goto end_with_files;
	}

	res->start = (unsigned char *)addr;

	// load all initrd
	p = res->start;

	for (size_t i = 0; i < conf->initrd.count; i++) {
		struct opened_file *file = &files[i];

		size = file->info->FileSize;
		es = file->file->Read(file->file, &size, p);

		if (EFI_ERROR(es)) {
			Print(L"Failed to read %D: %r\n", conf->initrd.paths[i], es);
			goto end_with_res;
		}

		p += size;
	}

	success = true;

	// clean up
end_with_res:
	free_pages(res);

end_with_files:
	for (size_t i = 0; i < conf->initrd.count; i++) {
		struct opened_file *file = &files[i];

		FreePool(file->info);

		if (file->file) {
			file->file->Close(file->file);
		}

		if (file->root) {
			file->root->Close(file->root);
		}
	}

	FreePool(files);

end:
	return success;
}

static
void
boot_linux(void)
{
	const struct config *conf = config_get();
	struct allocated_pages kernel, initrd;
	struct boot_params *params;
	EFI_STATUS es;

	// load kernel & initrd into memory
	if (!load_linux(&kernel)) {
		return;
	}

	if (CompareMem(kernel.start + 0x202, "HdrS", 4)) {
		Print(L"%D is not a Linux kernel or it is too old\n", conf->kernel);
		goto fail_with_kernel;
	}

	if (!load_initrd(&initrd)) {
		goto fail_with_kernel;
	}

	// prepare parameters
	params = AllocateZeroPool(sizeof(*params));

	if (!params) {
		Print(
			L"Failed to allocate %u bytes for boot parameters\n",
			sizeof(*params));
		goto fail_with_initrd;
	}

	CopyMem(
		&params->hdr,
		kernel.start + 0x01F1,
		(0x0202 + kernel.start[0x0201]) - 0x01F1);

	// clean up
	FreePool(params);

fail_with_initrd:
	free_pages(&initrd);

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
