#ifndef GW_LIFECYCLE_H
#define GW_LIFECYCLE_H

#include "gw_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Production Gateway boot lifecycle.
 *
 * EOL/manufacturing operation is intentionally excluded.
 */
typedef enum
{
    GW_LIFECYCLE_RESET = 0,
    GW_LIFECYCLE_PLATFORM_INIT,
    GW_LIFECYCLE_DRIVERS_INIT,
    GW_LIFECYCLE_RADIO_INIT,
    GW_LIFECYCLE_SERVICES_INIT,
    GW_LIFECYCLE_APPLICATION_READY,
    GW_LIFECYCLE_RUN
} GwLifecycleStage_t;

GwResult_t gwLifecycleInit(void);

GwResult_t gwLifecycleSetStage(GwLifecycleStage_t stage);

GwLifecycleStage_t gwLifecycleGetStage(void);

bool gwLifecycleIsRunning(void);

#ifdef __cplusplus
}
#endif

#endif /* GW_LIFECYCLE_H */
