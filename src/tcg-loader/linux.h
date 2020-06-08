#ifndef TCG_LOADER_LINUX_H
#define TCG_LOADER_LINUX_H

#include "page.h"

#include <stdbool.h>

bool linux_kernel_load(struct page_alloc *result);

bool linux_initrd_load(struct page_alloc *result);

void linux_boot(unsigned char *kernel, unsigned char *initrd);

#endif // TCG_LOADER_LINUX_H
