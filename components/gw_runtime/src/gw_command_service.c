#include "gw_command_service.h"

#include <stddef.h>

static bool s_initialized = false;

GwResult_t gwCommandServiceInit(void)
{
    if (s_initialized)
    {
        return GW_RESULT_ALREADY_INITIALIZED;
    }

    s_initialized = true;

    return GW_RESULT_OK;
}

GwResult_t gwCommandServiceDeinit(void)
{
    if (!s_initialized)
    {
        return GW_RESULT_NOT_INITIALIZED;
    }

    s_initialized = false;

    return GW_RESULT_OK;
}

GwResult_t gwCommandServiceSend(
    const GwCommandRequest_t *request)
{
    if (!s_initialized)
    {
        return GW_RESULT_NOT_INITIALIZED;
    }

    if (request == NULL)
    {
        return GW_RESULT_INVALID_ARG;
    }

    /*
     * Packet construction and radio transmission are intentionally
     * deferred to the lower protocol/platform integration layer.
     */

    return GW_RESULT_NOT_READY;
}

bool gwCommandServiceIsInitialized(void)
{
    return s_initialized;
}