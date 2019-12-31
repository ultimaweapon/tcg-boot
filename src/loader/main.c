#include <efi.h>
#include <efilib.h>

EFI_STATUS efi_main(EFI_HANDLE img, EFI_SYSTEM_TABLE *st)
{
	InitializeLib(img, st);
	Print(L"Hello, world!\n");
	return EFI_SUCCESS;
}
