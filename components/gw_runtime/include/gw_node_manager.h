#ifndef GW_NODE_MANAGER_H
#define GW_NODE_MANAGER_H

#include "gw_types.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Production Gateway Node runtime state.
 *
 * This is Gateway runtime state, not a wire-protocol state.
 */
typedef enum
{
    GW_NODE_STATE_REGISTERED = 0,
    GW_NODE_STATE_ONLINE,
    GW_NODE_STATE_OFFLINE,
    GW_NODE_STATE_FAULT
} GwNodeRuntimeState_t;

typedef struct
{
    uint8_t node_id;
    uint8_t gateway_id;
    GwNodeRuntimeState_t state;
    bool registered;
} GwNodeInfo_t;

GwResult_t gwNodeManagerInit(void);

GwResult_t gwNodeManagerDeinit(void);

GwResult_t gwNodeManagerRegister(
    uint8_t node_id,
    uint8_t gateway_id);

GwResult_t gwNodeManagerUnregister(
    uint8_t node_id);

GwResult_t gwNodeManagerGet(
    uint8_t node_id,
    GwNodeInfo_t *info);

GwResult_t gwNodeManagerSetState(
    uint8_t node_id,
    GwNodeRuntimeState_t state);

bool gwNodeManagerIsRegistered(
    uint8_t node_id);

size_t gwNodeManagerCount(void);

bool gwNodeManagerIsInitialized(void);

#ifdef __cplusplus
}
#endif

#endif /* GW_NODE_MANAGER_H */