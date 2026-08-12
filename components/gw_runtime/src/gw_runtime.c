#include "gw_runtime.h"

#include "gw_command_service.h"
#include "gw_event_service.h"
#include "gw_node_manager.h"

static bool s_initialized = false;

GwResult_t gwRuntimeDeinit(void)
{
    GwResult_t result;
    GwResult_t first_error = GW_RESULT_OK;

    if (!s_initialized)
    {
        return GW_RESULT_NOT_INITIALIZED;
    }

    result = gwEventServiceDeinit();
    if (result != GW_RESULT_OK && first_error == GW_RESULT_OK)
    {
        first_error = result;
    }

    result = gwCommandServiceDeinit();
    if (result != GW_RESULT_OK && first_error == GW_RESULT_OK)
    {
        first_error = result;
    }

    result = gwNodeManagerDeinit();
    if (result != GW_RESULT_OK && first_error == GW_RESULT_OK)
    {
        first_error = result;
    }

    s_initialized = false;

    return first_error;
}