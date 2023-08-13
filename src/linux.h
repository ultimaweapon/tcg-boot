#pragma once

#include "page.h"

#include <stdbool.h>
#include <stddef.h>

bool linux_kernel_load(struct page_alloc *result);

bool linux_initrd_load(struct page_alloc *result, size_t *len);

void linux_boot(unsigned char *kernel, unsigned char *initrd, size_t rdsize);
