#ifndef GW_PLATFORM_H
#define GW_PLATFORM_H

#include "gw_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Production Gateway platform lifecycle.
 *
 * Provides the hardware/platform initialization boundary
 * required by the Gateway core.
 *
 * EOL/manufacturing operation is intentionally excluded.
 */
GwResult_t gwPlatformInit(void);

GwResult_t gwPlatformDeinit(void);

bool gwPlatformIsInitialized(void);

#ifdef __cplusplus
}
#endif

#endif /* GW_PLATFORM_H */
