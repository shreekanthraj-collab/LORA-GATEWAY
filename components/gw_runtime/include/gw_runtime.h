#ifndef GW_RUNTIME_H
#define GW_RUNTIME_H

#include "gw_types.h"

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Production Gateway runtime coordinator.
 *
 * Coordinates Layer 3 runtime services:
 * - Node Manager
 * - Command Service
 * - Event Service
 *
 * Gateway boot lifecycle remains owned by gw_core.
 * EOL/manufacturing operation is intentionally excluded.
 */

GwResult_t gwRuntimeInit(void);

GwResult_t gwRuntimeDeinit(void);

bool gwRuntimeIsInitialized(void);

bool gwRuntimeIsReady(void);

/**
 * Process one received Gateway packet.
 *
 * The packet is received from the communication layer,
 * classified by wire packet type, and dispatched to the
 * appropriate Layer-3 event service.
 *
 * Transport ownership remains outside the runtime.
 */
GwResult_t gwRuntimeProcessRx(
    const uint8_t *data,
    size_t length);

#ifdef __cplusplus
}
#endif

#endif /* GW_RUNTIME_H */