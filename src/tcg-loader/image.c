#include "image.h"

#include "main.h"

#include <efilib.h>

EFI_LOADED_IMAGE_PROTOCOL * image_get_loaded(EFI_HANDLE img)
{
	EFI_STATUS res;
	EFI_LOADED_IMAGE_PROTOCOL *proto;

	res = uefi_call_wrapper(
		BS->OpenProtocol,
		6,
		img,
		&LoadedImageProtocol,
		(void **)&proto,
		tcg,
		NULL,
		EFI_OPEN_PROTOCOL_GET_PROTOCOL);

	return EFI_ERROR(res) ? NULL : proto;
}
