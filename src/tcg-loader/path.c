#include "path.h"

CHAR16 * path_get_file_path(EFI_DEVICE_PATH_PROTOCOL *proto)
{
	if (DevicePathType(proto) != MEDIA_DEVICE_PATH) {
		return NULL;
	}

	if (DevicePathSubType(proto) != MEDIA_FILEPATH_DP) {
		return NULL;
	}

	return ((FILEPATH_DEVICE_PATH *)proto)->PathName;
}
