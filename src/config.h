#pragma once

#include <efi/efi.h>

#include <stdbool.h>
#include <stddef.h>

struct config {
	EFI_DEVICE_PATH_PROTOCOL *kernel;
	char *command_line;
	struct {
		EFI_DEVICE_PATH_PROTOCOL **paths;
		size_t count;
	} initrd;
};

bool config_init(void);
void config_term(void);

const struct config * config_get(void);
