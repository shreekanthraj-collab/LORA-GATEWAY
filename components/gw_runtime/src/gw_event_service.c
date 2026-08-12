#include "gw_event_service.h"

static bool s_initialized = false;

GwResult_t gwEventServiceInit(void)
{
    if (s_initialized)
    {
        return GW_RESULT_ALREADY_INITIALIZED;
    }

    s_initialized = true;

    return GW_RESULT_OK;
}

GwResult_t gwEventServiceDeinit(void)
{
    if (!s_initialized)
    {
        return GW_RESULT_NOT_INITIALIZED;
    }

    s_initialized = false;

    return GW_RESULT_OK;
}

GwResult_t gwEventServiceProcess(
    const GwEventMessage_t *message)
{
    if (!s_initialized)
    {
        return GW_RESULT_NOT_INITIALIZED;
    }

    if (message == NULL)
    {
        return GW_RESULT_INVALID_ARG;
    }

    if (message->data == NULL || message->length == 0u)
    {
        return GW_RESULT_INVALID_ARG;
    }

    /*
     * Protocol decoding and CRC validation remain in gw_protocol.
     * Runtime event handling will be integrated after the Layer 3
     * service skeleton has been validated.
     */

    return GW_RESULT_NOT_READY;
}

bool gwEventServiceIsInitialized(void)
{
    return s_initialized;
}