#include "config.h"

#include "image.h"
#include "main.h"
#include "path.h"

#include <efi.h>
#include <efilib.h>

static int load(EFI_FILE_PROTOCOL *file)
{
	return 0;
}

int config_init(void)
{
	EFI_LOADED_IMAGE_PROTOCOL *app;
	CHAR16 *bin, *path;
	UINTN ps;
	EFI_FILE_PROTOCOL *vol, *file;
	EFI_STATUS es;
	int res;

	// get loaded image protocol
	app = image_get_loaded(tcg);

	if (!app) {
		Print(L"Failed to get loaded image protocol for the application\n");
		return 0;
	}

	// get configuration path
	bin = path_get_file_path(app->FilePath);

	if (!bin) {
		Print(
			L"This application does not support device %x:%x\n",
			app->FilePath->Type,
			app->FilePath->SubType);
		return 0;
	}

	ps = StrSize(bin) + sizeof(CHAR16) * 5; // size of '.conf'
	path = AllocatePool(ps);

	if (!path) {
		Print(L"Failed to allocate %u bytes\n", ps);
		return 0;
	}

	StrCpy(path, bin);
	StrCat(path, L".CONF");

	// get the volume of the image
	vol = LibOpenRoot(app->DeviceHandle);

	if (!vol) {
		Print(L"Failed to open volume that contains the application\n");
		goto fail;
	}

	// open config file
	es = uefi_call_wrapper(
		vol->Open,
		5,
		vol,
		&file,
		path,
		EFI_FILE_MODE_READ,
		0);

	if (EFI_ERROR(es)) {
		Print(L"Failed to open %s: %r\n", path, es);
		goto fail_with_vol;
	}

	// load config
	res = load(file);

	// clean up
	uefi_call_wrapper(file->Close, 1, file);
	uefi_call_wrapper(vol->Close, 1, vol);
	FreePool(path);

	return res;

fail_with_vol:
	uefi_call_wrapper(vol->Close, 1, vol);

fail:
	FreePool(path);

	return 0;
}

void config_term(void)
{
}
