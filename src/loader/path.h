#ifndef TCGBOOT_LOADER_PATH_H
#define TCGBOOT_LOADER_PATH_H

#include <efi.h>

CHAR16 * path_get_file_path(EFI_DEVICE_PATH_PROTOCOL *proto);

#endif // TCGBOOT_LOADER_PATH_H
