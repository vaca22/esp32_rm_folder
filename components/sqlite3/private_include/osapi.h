#ifndef __OSAPI_H__
#define __OSAPI_H__
// #include <stdio.h>
#include "esp_heap_caps.h"
// #include "esp_log.h"

// static const char *SQLITE_MEM_TAG = "osapi";

// #define os_malloc(x) malloc(x)
// #define os_realloc(x, y) realloc(x, y)
// #define os_free(x) free(x)


static inline void *_esp_malloc(size_t size)
{
// #if CONFIG_SPIRAM_BOOT_INIT
// 	void *ptr = heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
// 	ESP_LOGI(SQLITE_MEM_TAG, "Ex, %s, size: %d, %p", __func__, size, ptr);
// 	assert(ptr);
// 	if (ptr == NULL) {
// 		fprintf(stderr, "Failed allocated  memory\n");
// 	}
// 	return ptr;
// #else
// 	ESP_LOGI(SQLITE_MEM_TAG, "In, %s, size: %d", __func__, size);
// 	return malloc(size);
// #endif
	return NULL; 
}

static inline void _esp_free(void *ptr)
{
	// free(ptr);
}

static inline void *_esp_realloc(void *ptr, size_t size)
{

// #if CONFIG_SPIRAM_BOOT_INIT
// 	// ESP_LOGI(SQLITE_MEM_TAG, "Ex, %s, size: %d", __func__, size);
// 	return heap_caps_realloc(ptr, size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
// #else
// 	// ESP_LOGI(SQLITE_MEM_TAG, "In, %s, size: %d", __func__, size);
// 	return realloc(ptr, size);
// #endif
// 
	return NULL; 

}

#endif

