#ifndef GW_STORAGE_H
#define GW_STORAGE_H

#include "gw_types.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Production Gateway persistent-storage interface.
 *
 * Hardware/storage-specific implementation must remain
 * below this abstraction boundary.
 *
 * No NVS, LittleFS, SPIFFS, filesystem, or flash-driver
 * definitions belong here.
 */

GwResult_t gwStorageInit(void);

GwResult_t gwStorageDeinit(void);

GwResult_t gwStorageRead(const char *key,
                         uint8_t *buffer,
                         size_t buffer_size,
                         size_t *read_length);

GwResult_t gwStorageWrite(const char *key,
                          const uint8_t *data,
                          size_t length);

GwResult_t gwStorageErase(const char *key);

bool gwStorageIsInitialized(void);

#ifdef __cplusplus
}
#endif

#endif /* GW_STORAGE_H */