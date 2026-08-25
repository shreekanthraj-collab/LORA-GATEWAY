#include "gw_runtime.h"

#include "gw_command_service.h"
#include "gw_event_service.h"
#include "gw_node_manager.h"
#include "gw_protocol.h"

#include <stddef.h>

static bool s_initialized = false;

/* -------------------------------------------------------------------------- */
/* Runtime initialization                                                     */
/* -------------------------------------------------------------------------- */

GwResult_t gwRuntimeInit(void)
{
    GwResult_t result;

    if (s_initialized)
    {
        return GW_RESULT_ALREADY_INITIALIZED;
    }

    /* ---------------------------------------------------------------------- */
    /* Node Manager                                                           */
    /* ---------------------------------------------------------------------- */

    result = gwNodeManagerInit();

    if (result != GW_RESULT_OK)
    {
        return result;
    }

    /* ---------------------------------------------------------------------- */
    /* Command Service                                                        */
    /* ---------------------------------------------------------------------- */

    result = gwCommandServiceInit();

    if (result != GW_RESULT_OK)
    {
        (void)gwNodeManagerDeinit();
        return result;
    }

    /* ---------------------------------------------------------------------- */
    /* Event Service                                                          */
    /* ---------------------------------------------------------------------- */

    result = gwEventServiceInit();

    if (result != GW_RESULT_OK)
    {
        (void)gwCommandServiceDeinit();
        (void)gwNodeManagerDeinit();
        return result;
    }

    s_initialized = true;

    return GW_RESULT_OK;
}

/* -------------------------------------------------------------------------- */
/* Runtime deinitialization                                                   */
/* -------------------------------------------------------------------------- */

GwResult_t gwRuntimeDeinit(void)
{
    GwResult_t result;
    GwResult_t first_error = GW_RESULT_OK;

    if (!s_initialized)
    {
        return GW_RESULT_NOT_INITIALIZED;
    }

    /* ---------------------------------------------------------------------- */
    /* Event Service                                                          */
    /* ---------------------------------------------------------------------- */

    result = gwEventServiceDeinit();

    if (result != GW_RESULT_OK &&
        first_error == GW_RESULT_OK)
    {
        first_error = result;
    }

    /* ---------------------------------------------------------------------- */
    /* Command Service                                                        */
    /* ---------------------------------------------------------------------- */

    result = gwCommandServiceDeinit();

    if (result != GW_RESULT_OK &&
        first_error == GW_RESULT_OK)
    {
        first_error = result;
    }

    /* ---------------------------------------------------------------------- */
    /* Node Manager                                                           */
    /* ---------------------------------------------------------------------- */

    result = gwNodeManagerDeinit();

    if (result != GW_RESULT_OK &&
        first_error == GW_RESULT_OK)
    {
        first_error = result;
    }

    s_initialized = false;

    return first_error;
}

/* -------------------------------------------------------------------------- */
/* RX packet classification                                                   */
/* -------------------------------------------------------------------------- */

static GwResult_t gwRuntimeClassifyRx(
    const uint8_t *data,
    size_t length,
    GwEventMessage_t *message)
{
    if (data == NULL ||
        message == NULL)
    {
        return GW_RESULT_INVALID_ARG;
    }

    if (length < 2u)
    {
        return GW_RESULT_INVALID_ARG;
    }

    /*
     * Wire format:
     *
     * byte 0 = Node ID
     * byte 1 = packet type
     *
     * Runtime uses the protocol packet type only for
     * dispatch. Detailed packet validation remains in
     * gw_event_service / gw_protocol.
     */
    switch (data[1])
    {
        case GW_PKT_ACK:
            message->type = GW_EVENT_TYPE_ACK;
            break;

        case GW_PKT_STATUS:
            message->type = GW_EVENT_TYPE_STATUS;
            break;

        case GW_PKT_EVENT:
            message->type = GW_EVENT_TYPE_EVENT;
            break;

        case GW_PKT_SCHED:
            message->type = GW_EVENT_TYPE_SCHEDULE_STATUS;
            break;

        default:
            return GW_RESULT_INVALID_ARG;
    }

    message->data = data;
    message->length = length;

    return GW_RESULT_OK;
}

/* -------------------------------------------------------------------------- */
/* RX packet processing                                                       */
/* -------------------------------------------------------------------------- */

GwResult_t gwRuntimeProcessRx(
    const uint8_t *data,
    size_t length)
{
    GwEventMessage_t message;
    GwResult_t result;

    if (!s_initialized)
    {
        return GW_RESULT_NOT_INITIALIZED;
    }

    if (data == NULL ||
        length == 0u)
    {
        return GW_RESULT_INVALID_ARG;
    }

    message.type = GW_EVENT_TYPE_ACK;
    message.data = NULL;
    message.length = 0u;

    result = gwRuntimeClassifyRx(
        data,
        length,
        &message);

    if (result != GW_RESULT_OK)
    {
        return result;
    }

    return gwEventServiceProcess(
        &message);
}

/* -------------------------------------------------------------------------- */
/* Runtime state                                                              */
/* -------------------------------------------------------------------------- */

bool gwRuntimeIsInitialized(void)
{
    return s_initialized;
}

bool gwRuntimeIsReady(void)
{
    return s_initialized &&
           gwNodeManagerIsInitialized() &&
           gwCommandServiceIsInitialized() &&
           gwEventServiceIsInitialized();
}