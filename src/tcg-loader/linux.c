#include "linux.h"

#include "config.h"
#include "page.h"
#include "path.h"

#include <efi.h>
#include <efilib.h>

#include <stdbool.h>
#include <stddef.h>

bool
linux_kernel_load(struct page_alloc *result)
{
	const struct config *conf = config_get();
	bool success = false;
	EFI_DEVICE_PATH_PROTOCOL *path;
	EFI_FILE_PROTOCOL *root, *file;
	EFI_STATUS es;
	EFI_FILE_INFO *info;
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
	ZeroMem(result, sizeof(*result));

	result->count = EFI_SIZE_TO_PAGES(info->FileSize);
	result->type = EfiLoaderData;
#if defined(__i386__) || defined(__amd64__)
	result->addr = (unsigned char *)0xFFFFFFFF;
#endif

	if (!page_alloc(
#if defined(__i386__) || defined(__amd64__)
		AllocateMaxAddress,
#else
		AllocateAnyPages,
#endif
		result)) {
		goto end_with_info;
	}

	size = info->FileSize;
	es = file->Read(file, &size, result->addr);

	if (EFI_ERROR(es)) {
		Print(L"Failed to read %D: %r\n", conf->kernel, es);
		goto end_with_result;
	}

	if (size < (0x202 + 4) || CompareMem(result->addr + 0x202, "HdrS", 4)) {
		Print(L"%D is not a Linux kernel or it is too old\n", conf->kernel);
		goto end_with_result;
	}

	success = true;

	// clean up
end_with_result:
	if (!success) {
		page_free(result);
	}

end_with_info:
	FreePool(info);

end_with_file:
	es = file->Close(file);

	if (EFI_ERROR(es)) {
		Print(L"Failed to close %D: %r\n", conf->kernel, es);
	}

end_with_root:
	es = root->Close(root);

	if (EFI_ERROR(es)) {
		Print(L"Failed to close the volume of %D: %r\n", conf->kernel, es);
	}

end:
	return success;
}

bool
linux_initrd_load(struct page_alloc *result, size_t *len)
{
	const struct config *conf = config_get();
	bool success = false;
	UINTN size;
	struct opened_file {
		EFI_FILE_PROTOCOL *root, *file;
		EFI_FILE_INFO *info;
	} *files;
	EFI_STATUS es;

	// prepare to load all initrd
	if (!conf->initrd.count) {
		ZeroMem(result, sizeof(*result));
		*len = 0;
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
		ZeroMem(result, sizeof(*result));
		*len = 0;
		success = true;
		goto end_with_files;
	}

	result->count = EFI_SIZE_TO_PAGES(size);
	result->type = EfiLoaderData;
#if defined(__i386__) || defined(__amd64__)
	result->addr = (unsigned char *)0xFFFFFFFF;
#endif

	if (!page_alloc(
#if defined(__i386__) || defined(__amd64__)
		AllocateMaxAddress,
#else
		AllocateAnyPages,
#endif
		result)) {
		goto end_with_files;
	}

	// load all initrd
	*len = 0;

	for (size_t i = 0; i < conf->initrd.count; i++) {
		struct opened_file *file = &files[i];

		size = file->info->FileSize;
		es = file->file->Read(file->file, &size, result->addr + *len);

		if (EFI_ERROR(es)) {
			Print(L"Failed to read %D: %r\n", conf->initrd.paths[i], es);
			goto end_with_result;
		}

		*len += size;
	}

	success = true;

	// clean up
end_with_result:
	page_free(result);

end_with_files:
	for (size_t i = 0; i < conf->initrd.count; i++) {
		struct opened_file *file = &files[i];

		FreePool(file->info);

		if (file->file) {
			es = file->file->Close(file->file);

			if (EFI_ERROR(es)) {
				Print(L"Failed to close %D: %r\n", conf->initrd.paths[i], es);
			}
		}

		if (file->root) {
			es = file->root->Close(file->root);

			if (EFI_ERROR(es)) {
				Print(
					L"Failed to close the volume of %D: %r\n",
					conf->initrd.paths[i],
					es);
			}
		}
	}

	FreePool(files);

end:
	return success;
}
