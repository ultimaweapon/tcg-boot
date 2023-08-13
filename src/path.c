#include "path.h"

#include "main.h"

#include <efi/efilib.h>

void * path_get_device_protocol(
	EFI_DEVICE_PATH_PROTOCOL **path,
	EFI_GUID *proto)
{
	EFI_STATUS es;
	EFI_HANDLE dev;
	void *i;

	es = BS->LocateDevicePath(proto, path, &dev);

	if (EFI_ERROR(es)) {
		return NULL;
	}

	es = BS->OpenProtocol(
		dev,
		proto,
		&i,
		tcg,
		NULL,
		EFI_OPEN_PROTOCOL_GET_PROTOCOL
	);

	return EFI_ERROR(es) ? NULL : i;
}

CHAR16 * path_get_file_path(EFI_DEVICE_PATH_PROTOCOL *proto)
{
	if (DevicePathType(proto) != MEDIA_DEVICE_PATH) {
		return NULL;
	}

	if (DevicePathSubType(proto) != MEDIA_FILEPATH_DP) {
		return NULL;
	}

	return ((FILEPATH_DEVICE_PATH *)proto)->PathName;
}

EFI_FILE_PROTOCOL * path_open_device_volume(EFI_DEVICE_PATH_PROTOCOL **path)
{
	EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fs;
	EFI_STATUS es;
	EFI_FILE_PROTOCOL *root;

	fs = path_get_device_protocol(path, &FileSystemProtocol);

	if (!fs) {
		return NULL;
	}

	es = fs->OpenVolume(fs, &root);

	return EFI_ERROR(es) ? NULL : root;
}
