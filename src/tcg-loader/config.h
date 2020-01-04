#ifndef TCG_LOADER_CONFIG_H
#define TCG_LOADER_CONFIG_H

#include <efi.h>

#include <stdbool.h>

struct config {
	EFI_DEVICE_PATH_PROTOCOL *kernel;
};

bool config_init(void);
void config_term(void);

const struct config * config_get(void);

#endif // TCG_LOADER_CONFIG_H
