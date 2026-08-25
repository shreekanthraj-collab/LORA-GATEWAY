#include "gw_node_manager.h"
#include "gw_config.h"

#include <string.h>

static GwNodeInfo_t s_nodes[GW_CONFIG_MAX_NODES];
static GwNodeTelemetry_t s_telemetry[GW_CONFIG_MAX_NODES];

static bool s_initialized = false;
static size_t s_count = 0u;

/* -------------------------------------------------------------------------- */
/* Lifecycle                                                                  */
/* -------------------------------------------------------------------------- */

GwResult_t gwNodeManagerInit(void)
{
    if (s_initialized)
    {
        return GW_RESULT_ALREADY_INITIALIZED;
    }

    memset(s_nodes, 0, sizeof(s_nodes));
    memset(s_telemetry, 0, sizeof(s_telemetry));

    s_count = 0u;
    s_initialized = true;

    return GW_RESULT_OK;
}

GwResult_t gwNodeManagerDeinit(void)
{
    if (!s_initialized)
    {
        return GW_RESULT_NOT_INITIALIZED;
    }

    memset(s_nodes, 0, sizeof(s_nodes));
    memset(s_telemetry, 0, sizeof(s_telemetry));

    s_count = 0u;
    s_initialized = false;

    return GW_RESULT_OK;
}

/* -------------------------------------------------------------------------- */
/* Node registration                                                           */
/* -------------------------------------------------------------------------- */

GwResult_t gwNodeManagerRegister(
    uint8_t node_id,
    uint8_t gateway_id)
{
    if (!s_initialized)
    {
        return GW_RESULT_NOT_INITIALIZED;
    }

    /*
     * node_id is uint8_t and GW_CONFIG_MAX_NODES is 256.
     *
     * Therefore every possible uint8_t node ID (0..255) is a valid
     * array index for the configured node table.
     */

    if (s_nodes[node_id].registered)
    {
        return GW_RESULT_BUSY;
    }

    s_nodes[node_id].node_id = node_id;
    s_nodes[node_id].gateway_id = gateway_id;
    s_nodes[node_id].state = GW_NODE_STATE_REGISTERED;
    s_nodes[node_id].registered = true;

    memset(
        &s_telemetry[node_id],
        0,
        sizeof(s_telemetry[node_id]));

    s_count++;

    return GW_RESULT_OK;
}

GwResult_t gwNodeManagerUnregister(
    uint8_t node_id)
{
    if (!s_initialized)
    {
        return GW_RESULT_NOT_INITIALIZED;
    }

    if (!s_nodes[node_id].registered)
    {
        return GW_RESULT_ERROR;
    }

    memset(
        &s_nodes[node_id],
        0,
        sizeof(s_nodes[node_id]));

    memset(
        &s_telemetry[node_id],
        0,
        sizeof(s_telemetry[node_id]));

    if (s_count > 0u)
    {
        s_count--;
    }

    return GW_RESULT_OK;
}

/* -------------------------------------------------------------------------- */
/* Node information                                                           */
/* -------------------------------------------------------------------------- */

GwResult_t gwNodeManagerGet(
    uint8_t node_id,
    GwNodeInfo_t *info)
{
    if (!s_initialized)
    {
        return GW_RESULT_NOT_INITIALIZED;
    }

    if (info == NULL)
    {
        return GW_RESULT_INVALID_ARG;
    }

    if (!s_nodes[node_id].registered)
    {
        return GW_RESULT_ERROR;
    }

    *info = s_nodes[node_id];

    return GW_RESULT_OK;
}

GwResult_t gwNodeManagerSetState(
    uint8_t node_id,
    GwNodeRuntimeState_t state)
{
    if (!s_initialized)
    {
        return GW_RESULT_NOT_INITIALIZED;
    }

    if (!s_nodes[node_id].registered)
    {
        return GW_RESULT_ERROR;
    }

    if (state < GW_NODE_STATE_REGISTERED ||
        state > GW_NODE_STATE_FAULT)
    {
        return GW_RESULT_INVALID_ARG;
    }

    s_nodes[node_id].state = state;

    return GW_RESULT_OK;
}

/* -------------------------------------------------------------------------- */
/* Telemetry                                                                  */
/* -------------------------------------------------------------------------- */

GwResult_t gwNodeManagerUpdateTelemetry(
    uint8_t node_id,
    const GwNodeTelemetry_t *telemetry)
{
    if (!s_initialized)
    {
        return GW_RESULT_NOT_INITIALIZED;
    }

    if (telemetry == NULL)
    {
        return GW_RESULT_INVALID_ARG;
    }

    if (!s_nodes[node_id].registered)
    {
        return GW_RESULT_ERROR;
    }

    s_telemetry[node_id] = *telemetry;
    s_telemetry[node_id].valid = true;

    return GW_RESULT_OK;
}

GwResult_t gwNodeManagerGetTelemetry(
    uint8_t node_id,
    GwNodeTelemetry_t *telemetry)
{
    if (!s_initialized)
    {
        return GW_RESULT_NOT_INITIALIZED;
    }

    if (telemetry == NULL)
    {
        return GW_RESULT_INVALID_ARG;
    }

    if (!s_nodes[node_id].registered)
    {
        return GW_RESULT_ERROR;
    }

    *telemetry = s_telemetry[node_id];

    return GW_RESULT_OK;
}

/* -------------------------------------------------------------------------- */
/* Queries                                                                    */
/* -------------------------------------------------------------------------- */

bool gwNodeManagerIsRegistered(
    uint8_t node_id)
{
    return s_initialized &&
           s_nodes[node_id].registered;
}

size_t gwNodeManagerCount(void)
{
    return s_count;
}

bool gwNodeManagerIsInitialized(void)
{
    return s_initialized;
}