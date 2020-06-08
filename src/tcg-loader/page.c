#include "page.h"

#include <efi.h>
#include <efilib.h>

bool
page_alloc(EFI_ALLOCATE_TYPE loc, struct page_alloc *info)
{
	EFI_STATUS es;
	EFI_PHYSICAL_ADDRESS addr;

	addr = (EFI_PHYSICAL_ADDRESS)info->addr;
	es = BS->AllocatePages(loc, info->type, info->count, &addr);

	if (EFI_ERROR(es)) {
		Print(L"Failed to allocated %u pages: %r\n", info->count, es);
		return false;
	}

	info->addr = (unsigned char *)addr;

	return true;
}

void
page_free(struct page_alloc *page)
{
	EFI_STATUS es;

	if (!page->count) {
		return;
	}

	es = BS->FreePages((EFI_PHYSICAL_ADDRESS)page->addr, page->count);

	if (EFI_ERROR(es)) {
		Print(
			L"Failed to free %u pages starting at %lX: %r\n",
			page->count,
			page->addr,
			es);
		return;
	}

	ZeroMem(page, sizeof(*page));
}
