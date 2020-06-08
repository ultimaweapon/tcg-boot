#ifndef TCG_LOADER_PAGE_H
#define TCG_LOADER_PAGE_H

#include <efi.h>

#include <stdbool.h>
#include <stddef.h>

struct page_alloc {
	unsigned char *addr;
	size_t count;
	EFI_MEMORY_TYPE type;
};

bool page_alloc(EFI_ALLOCATE_TYPE loc, struct page_alloc *info);
void page_free(struct page_alloc *page);

#endif // TCG_LOADER_PAGE_H
