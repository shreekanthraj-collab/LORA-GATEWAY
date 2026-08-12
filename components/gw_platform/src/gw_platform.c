#include "gw_platform.h"

static bool s_platform_initialized = false;

GwResult_t gwPlatformInit(void)
{
    if (s_platform_initialized)
    {
        return GW_RESULT_ALREADY_INITIALIZED;
    }

    s_platform_initialized = true;

    return GW_RESULT_OK;
}

GwResult_t gwPlatformDeinit(void)
{
    if (!s_platform_initialized)
    {
        return GW_RESULT_NOT_INITIALIZED;
    }

    s_platform_initialized = false;

    return GW_RESULT_OK;
}

bool gwPlatformIsInitialized(void)
{
    return s_platform_initialized;
}