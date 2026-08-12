#include "gw_lifecycle.h"

static GwLifecycleStage_t s_lifecycle_stage = GW_LIFECYCLE_RESET;
static bool s_lifecycle_initialized = false;

GwResult_t gwLifecycleInit(void)
{
    if (s_lifecycle_initialized)
    {
        return GW_RESULT_ALREADY_INITIALIZED;
    }

    s_lifecycle_stage = GW_LIFECYCLE_PLATFORM_INIT;
    s_lifecycle_initialized = true;

    return GW_RESULT_OK;
}

GwResult_t gwLifecycleSetStage(GwLifecycleStage_t stage)
{
    if (!s_lifecycle_initialized)
    {
        return GW_RESULT_NOT_INITIALIZED;
    }

    if (stage < GW_LIFECYCLE_RESET ||
        stage > GW_LIFECYCLE_RUN)
    {
        return GW_RESULT_INVALID_ARG;
    }

    if (stage < s_lifecycle_stage)
    {
        return GW_RESULT_INVALID_ARG;
    }

    s_lifecycle_stage = stage;

    return GW_RESULT_OK;
}
GwLifecycleStage_t gwLifecycleGetStage(void)
{
    return s_lifecycle_stage;
}

bool gwLifecycleIsRunning(void)
{
    return s_lifecycle_initialized &&
           (s_lifecycle_stage == GW_LIFECYCLE_RUN);
}
