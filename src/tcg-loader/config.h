#ifndef TCG_LOADER_CONFIG_H
#define TCG_LOADER_CONFIG_H

#include <efi.h>

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

#endif // TCG_LOADER_CONFIG_H
