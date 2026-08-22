
#include "gw_radio.h"
#include "gw_radio_hw.h"

#include <string.h>

static bool s_radio_initialized[GW_RADIO_COUNT] = { false };
static GwRadioConfig_t s_radio_config;

static bool gwRadioIdValid(GwRadioId_t radio)
{
    return (radio < GW_RADIO_COUNT);
}

GwResult_t gwRadioInit(const GwRadioConfig_t *config)
{
    if (config == NULL)
    {
        return GW_RESULT_INVALID_ARG;
    }

    if (config->frequency_hz < 850000000UL ||
        config->frequency_hz > 930000000UL ||
        config->spreading_factor < 5U ||
        config->spreading_factor > 12U ||
        config->coding_rate < 1U ||
        config->coding_rate > 4U)
    {
        return GW_RESULT_INVALID_ARG;
    }

    if (gwRadioAllInitialized())
    {
        return GW_RESULT_ALREADY_INITIALIZED;
    }

    memcpy(&s_radio_config, config, sizeof(s_radio_config));

    /*
     * Hardware initialization will be connected here.
     *
     * Both radios share:
     *   MOSI = GW_RADIO_SPI_MOSI_GPIO
     *   MISO = GW_RADIO_SPI_MISO_GPIO
     *   SCK  = GW_RADIO_SPI_SCK_GPIO
     *
     * Each radio has independent control lines.
     */

    for (size_t i = 0U; i < GW_RADIO_COUNT; ++i)
    {
        s_radio_initialized[i] = true;
    }

    return GW_RESULT_OK;
}

GwResult_t gwRadioDeinit(void)
{
    if (!gwRadioAllInitialized())
    {
        bool any_initialized = false;

        for (size_t i = 0U; i < GW_RADIO_COUNT; ++i)
        {
            if (s_radio_initialized[i])
            {
                any_initialized = true;
                break;
            }
        }

        if (!any_initialized)
        {
            return GW_RESULT_NOT_INITIALIZED;
        }
    }

    for (size_t i = 0U; i < GW_RADIO_COUNT; ++i)
    {
        s_radio_initialized[i] = false;
    }

    memset(&s_radio_config, 0, sizeof(s_radio_config));

    return GW_RESULT_OK;
}

GwResult_t gwRadioSend(
    GwRadioId_t radio,
    const uint8_t *data,
    size_t length)
{
    if (!gwRadioIdValid(radio) ||
        (data == NULL && length != 0U))
    {
        return GW_RESULT_INVALID_ARG;
    }

    if (!s_radio_initialized[radio])
    {
        return GW_RESULT_NOT_INITIALIZED;
    }

    /*
     * SX1262 transmit implementation will be connected here.
     */
    return GW_RESULT_NOT_READY;
}

GwResult_t gwRadioReceive(
    GwRadioId_t radio,
    uint8_t *buffer,
    size_t buffer_size,
    size_t *received_length)
{
    if (!gwRadioIdValid(radio) ||
        buffer == NULL ||
        buffer_size == 0U ||
        received_length == NULL)
    {
        return GW_RESULT_INVALID_ARG;
    }

    if (!s_radio_initialized[radio])
    {
        return GW_RESULT_NOT_INITIALIZED;
    }

    *received_length = 0U;

    /*
     * SX1262 receive implementation will be connected here.
     */
    return GW_RESULT_NOT_READY;
}

bool gwRadioIsInitialized(GwRadioId_t radio)
{
    return gwRadioIdValid(radio) &&
           s_radio_initialized[radio];
}

bool gwRadioAllInitialized(void)
{
    for (size_t i = 0U; i < GW_RADIO_COUNT; ++i)
    {
        if (!s_radio_initialized[i])
        {
            return false;
        }
    }

    return true;
}

GwResult_t gwRadioGetConfig(GwRadioConfig_t *config)
{
    if (config == NULL)
    {
        return GW_RESULT_INVALID_ARG;
    }

    memcpy(config, &s_radio_config, sizeof(s_radio_config));

    return GW_RESULT_OK;
}
