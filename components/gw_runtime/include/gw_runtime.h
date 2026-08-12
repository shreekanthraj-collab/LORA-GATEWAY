#ifndef GW_RUNTIME_H
#define GW_RUNTIME_H

#include "gw_types.h"

#include <stdbool.h>

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

#ifdef __cplusplus
}
#endif

#endif /* GW_RUNTIME_H */