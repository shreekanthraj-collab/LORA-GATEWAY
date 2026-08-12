#ifndef GW_EVENT_SERVICE_H
#define GW_EVENT_SERVICE_H

#include "gw_types.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Production Gateway incoming event service.
 *
 * Accepts validated protocol messages at the
 * Layer 3 runtime boundary.
 *
 * Protocol packet definitions remain in gw_protocol.
 * CRC validation remains in gw_protocol.
 *
 * EOL/manufacturing operation is intentionally excluded.
 */
typedef enum
{
    GW_EVENT_TYPE_ACK = 0,
    GW_EVENT_TYPE_STATUS,
    GW_EVENT_TYPE_EVENT,
    GW_EVENT_TYPE_SCHEDULE_STATUS,
    GW_EVENT_TYPE_REBIND
} GwEventType_t;

typedef struct
{
    GwEventType_t type;
    const uint8_t *data;
    size_t length;
} GwEventMessage_t;

GwResult_t gwEventServiceInit(void);

GwResult_t gwEventServiceDeinit(void);

GwResult_t gwEventServiceProcess(
    const GwEventMessage_t *message);

bool gwEventServiceIsInitialized(void);

#ifdef __cplusplus
}
#endif

#endif /* GW_EVENT_SERVICE_H */