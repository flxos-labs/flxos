#ifndef STORAGE_MOCKS_H
#define STORAGE_MOCKS_H

#include "esp_err.h"

typedef int wl_handle_t;
#define WL_INVALID_HANDLE -1

typedef struct {
	bool format_if_mount_failed;
	int max_files;
	int allocation_unit_size;
	bool disk_status_check_enable;
	bool use_one_fat;
} esp_vfs_fat_mount_config_t;

inline esp_err_t esp_vfs_fat_spiflash_mount_rw_wl(const char* base_path, const char* partition_label, const esp_vfs_fat_mount_config_t* mount_config, wl_handle_t* wl_handle) {
	*wl_handle = 1;
	return ESP_OK;
}

#endif
