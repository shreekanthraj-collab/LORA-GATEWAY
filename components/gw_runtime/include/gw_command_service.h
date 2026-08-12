#ifndef GW_COMMAND_SERVICE_H
#define GW_COMMAND_SERVICE_H

#include "gw_types.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Production Gateway outgoing command service.
 *
 * Provides a generic runtime command boundary.
 * Protocol packet construction remains below this interface.
 *
 * EOL/manufacturing operation is intentionally excluded.
 */
typedef struct
{
    uint8_t node;
    uint8_t command;
    uint8_t value;
} GwCommandRequest_t;

GwResult_t gwCommandServiceInit(void);

GwResult_t gwCommandServiceDeinit(void);

GwResult_t gwCommandServiceSend(
    const GwCommandRequest_t *request);

bool gwCommandServiceIsInitialized(void);

#ifdef __cplusplus
}
#endif

#endif /* GW_COMMAND_SERVICE_H */