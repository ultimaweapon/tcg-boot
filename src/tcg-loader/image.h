#ifndef TCG_LOADER_IMAGE_H
#define TCG_LOADER_IMAGE_H

#include <efi.h>

EFI_LOADED_IMAGE_PROTOCOL * image_get_loaded(EFI_HANDLE img);

#endif // TCG_LOADER_IMAGE_H
