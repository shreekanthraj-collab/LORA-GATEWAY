#include "gw_runtime.h"

#include "gw_command_service.h"
#include "gw_event_service.h"
#include "gw_node_manager.h"

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