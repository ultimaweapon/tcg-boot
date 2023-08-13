#pragma once

#include <efi/efi.h>

void * path_get_device_protocol(
	EFI_DEVICE_PATH_PROTOCOL **path,
	EFI_GUID *proto);
CHAR16 * path_get_file_path(EFI_DEVICE_PATH_PROTOCOL *proto);
EFI_FILE_PROTOCOL * path_open_device_volume(EFI_DEVICE_PATH_PROTOCOL **path);
