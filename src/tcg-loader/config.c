#include "config.h"

#include "image.h"
#include "main.h"
#include "path.h"

#include <efi.h>
#include <efilib.h>

#include <stdbool.h>
#include <stddef.h>

static bool process(char *key, char *val, size_t line)
{
	return true;
}

static bool parse(char *data)
{
	char *next, *key;
	size_t line;

	key = NULL;
	line = 1;

	for (next = data;; next++) {
		char ch = *next;
		size_t len;

		// move until we found a separator
		switch (ch) {
		case '=':
			if (key) {
				continue;
			}
		case '\n':
		case 0:
			break;
		default:
			continue;
		}

		len = next - data;

		// process
		if (!key && len) {
			if (ch != '=') {
				goto invalid;
			}
			key = data;
			*next = 0;
		} else if (!key && !len) {
			if (ch == '=') {
				// line begin with =
				goto invalid;
			}
		} else {
			*next = 0;
			if (!process(key, data, line)) {
				return false;
			}
			key = NULL;
		}

		// decide what to do next
		if (!ch) {
			break;
		} else if (ch == '\n') {
			line++;
		}

		data = next + 1;
	}

	return true;

invalid:
	Print(L"Invalid configuration at line %u\n", line);

	return false;
}

static bool load(EFI_FILE_PROTOCOL *file)
{
	EFI_FILE_INFO *info;
	UINTN size;
	char *data;
	EFI_STATUS es;
	bool res;

	// allocate buffer to read file
	info = LibFileInfo(file);

	if (!info) {
		Print(L"Failed to allocate buffer for configuration file's info\n");
		return false;
	}

	size = info->FileSize + 1;
	FreePool(info);

	data = AllocatePool(size);

	if (!data) {
		Print(L"Failed to allocate %u bytes\n", size);
		return false;
	}

	// read file
	es = file->Read(file, &size, data);

	if (EFI_ERROR(es)) {
		Print(L"Failed to read configuration file: %r\n", es);
		goto fail;
	}

	data[size] = 0; // null terminate

	// parse
	res = parse(data);

	// clean up
	FreePool(data);

	return res;

fail:
	FreePool(data);

	return false;
}

bool config_init(void)
{
	EFI_LOADED_IMAGE_PROTOCOL *app;
	CHAR16 *bin, *path;
	UINTN ps;
	EFI_FILE_PROTOCOL *vol, *file;
	EFI_STATUS es;
	bool res;

	// get loaded image protocol
	app = image_get_loaded(tcg);

	if (!app) {
		Print(L"Failed to get loaded image protocol for the application\n");
		return false;
	}

	// get configuration path
	bin = path_get_file_path(app->FilePath);

	if (!bin) {
		Print(
			L"This application does not support device %x:%x\n",
			app->FilePath->Type,
			app->FilePath->SubType);
		return false;
	}

	ps = StrSize(bin) + sizeof(CHAR16) * 5; // size of '.conf'
	path = AllocatePool(ps);

	if (!path) {
		Print(L"Failed to allocate %u bytes\n", ps);
		return false;
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
	es = vol->Open(vol, &file, path, EFI_FILE_MODE_READ, 0);

	if (EFI_ERROR(es)) {
		Print(L"Failed to open %s: %r\n", path, es);
		goto fail_with_vol;
	}

	// load config
	res = load(file);

	// clean up
	file->Close(file);
	vol->Close(vol);
	FreePool(path);

	return res;

fail_with_vol:
	vol->Close(vol);

fail:
	FreePool(path);

	return false;
}

void config_term(void)
{
}
