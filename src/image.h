#pragma once

#include <efi/efi.h>

EFI_LOADED_IMAGE_PROTOCOL * image_get_loaded(EFI_HANDLE img);
