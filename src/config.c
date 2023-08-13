#include "config.h"

#include "image.h"
#include "main.h"
#include "path.h"

#include <efi/efi.h>
#include <efi/efilib.h>

#include <stdbool.h>
#include <stddef.h>

static struct config conf;

static void free(struct config *conf)
{
	for (size_t i = 0; i < conf->initrd.count; i++) {
		FreePool(conf->initrd.paths[i]);
	}

	FreePool(conf->kernel);
	FreePool(conf->command_line);
	FreePool(conf->initrd.paths);

	ZeroMem(conf, sizeof(*conf));
}

static void print_duplicate(size_t line)
{
	Print(L"Duplicated configuration at line %u\n", line);
}

static EFI_DEVICE_PATH_PROTOCOL * parse_path(const char *path, size_t line)
{
	EFI_LOADED_IMAGE_PROTOCOL *app;
	CHAR16 *unicode;
	EFI_DEVICE_PATH_PROTOCOL *protocol;

	if (!*path) {
		Print(L"Invalid path name at line %u\n", line);
		return NULL;
	}

	// get the loaded image protocol of the application to find the base path
	// of the linux
	app = image_get_loaded(tcg);

	if (!app) {
		Print(L"Failed to get loaded image protocol for the application\n");
		return NULL;
	}

	// convert ascii path to native path
	unicode = PoolPrint(L"%a", path);

	if (!unicode) {
		Print(L"Failed to convert path %a to the unicode string\n", path);
		return NULL;
	}

	// replace forward slash with back slash
	for (CHAR16 *p = unicode; *p; p++) {
		if (*p == '/') {
			*p = '\\';
		}
	}

	// get full path
	protocol = FileDevicePath(app->DeviceHandle, unicode);
	FreePool(unicode);

	if (!protocol) {
		Print(L"Failed to get device path for %a\n", path);
		return NULL;
	}

	return protocol;
}

static bool parse_kernel(char *val, size_t line, struct config *conf)
{
	if (conf->kernel) {
		print_duplicate(line);
		return false;
	}

	conf->kernel = parse_path(val, line);

	return conf->kernel ? true : false;
}

static bool parse_command_line(char *val, size_t line, struct config *conf)
{
	UINTN size;

	if (conf->command_line) {
		print_duplicate(line);
		return false;
	}

	size = strlena(val) + 1;
	conf->command_line = AllocatePool(size);

	if (!conf->command_line) {
		Print(
			L"Failed to allocate %u bytes for the value at line %u\n",
			size,
			line);
		return false;
	}

	CopyMem(conf->command_line, val, size);

	return true;
}

static bool parse_initrd(char *val, size_t line, struct config *conf)
{
	EFI_DEVICE_PATH_PROTOCOL *path;
	size_t size;

	// convert local path to device path
	path = parse_path(val, line);

	if (!path) {
		return false;
	}

	// put to the end of the list
	size = sizeof(*conf->initrd.paths) * conf->initrd.count;
	conf->initrd.paths = ReallocatePool(
		conf->initrd.paths,
		size,
		size + sizeof(*conf->initrd.paths));

	if (!conf->initrd.paths) {
		Print(L"Failed grow an array for a new item at line %u\n", line);
		FreePool(path);
		return false;
	}

	conf->initrd.paths[conf->initrd.count++] = path;

	return true;
}

static bool process(char *key, char *val, size_t line, struct config *conf)
{
	if (strcmpa(key, "kernel") == 0) {
		return parse_kernel(val, line, conf);
	} else if (strcmpa(key, "command_line") == 0) {
		return parse_command_line(val, line, conf);
	} else if (strcmpa(key, "initrd") == 0) {
		return parse_initrd(val, line, conf);
	} else {
		Print(L"Unknow configuration '%a' at line %u\n", key, line);
		return false;
	}

	return true;
}

static bool parse(char *data, struct config *conf)
{
	char *next, *key;
	size_t line;

	// parse
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
			if (!process(key, data, line, conf)) {
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
	free(&conf);
	res = parse(data, &conf);
	FreePool(data);

	if (!res) {
		free(&conf);
	}

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
	free(&conf);
}

const struct config * config_get(void)
{
	return &conf;
}
