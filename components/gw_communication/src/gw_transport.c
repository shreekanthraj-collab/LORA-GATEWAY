#include "gw_transport.h"

#include "gw_radio.h"

#include <string.h>

#define GW_TRANSPORT_SIM_BUFFER_SIZE 256u

static bool s_initialized = false;
static GwTransportConfig_t s_config;

static uint8_t s_sim_rx_buffer[GW_TRANSPORT_SIM_BUFFER_SIZE];
static size_t s_sim_rx_length = 0u;
static bool s_sim_rx_pending = false;

/* -------------------------------------------------------------------------- */
/* Internal validation                                                        */
/* -------------------------------------------------------------------------- */

static bool gwTransportConfigValid(
    const GwTransportConfig_t *config)
{
    if (config == NULL)
    {
        return false;
    }

    if (config->type >= GW_TRANSPORT_COUNT)
    {
        return false;
    }

    if (config->type == GW_TRANSPORT_RADIO)
    {
        if (config->radio > GW_TRANSPORT_RADIO_AUTO)
        {
            return false;
        }
    }

    return true;
}

/* -------------------------------------------------------------------------- */
/* SIM transport                                                              */
/* -------------------------------------------------------------------------- */

static GwResult_t gwTransportSimSend(
    const uint8_t *data,
    size_t length)
{
    if (data == NULL || length == 0u)
    {
        return GW_RESULT_INVALID_ARG;
    }

    if (length > GW_TRANSPORT_SIM_BUFFER_SIZE)
    {
        return GW_RESULT_INVALID_ARG;
    }

    /*
     * Simulation loopback:
     *
     * TX immediately becomes RX data.
     *
     * This keeps the communication layer testable without
     * requiring SX1262 hardware.
     */
    memcpy(
        s_sim_rx_buffer,
        data,
        length);

    s_sim_rx_length = length;
    s_sim_rx_pending = true;

    return GW_RESULT_OK;
}

static GwResult_t gwTransportSimReceive(
    uint8_t *buffer,
    size_t buffer_size,
    size_t *received_length)
{
    if (buffer == NULL ||
        received_length == NULL ||
        buffer_size == 0u)
    {
        return GW_RESULT_INVALID_ARG;
    }

    *received_length = 0u;

    if (!s_sim_rx_pending)
    {
        return GW_RESULT_NOT_READY;
    }

    if (s_sim_rx_length > buffer_size)
    {
        return GW_RESULT_INVALID_ARG;
    }

    memcpy(
        buffer,
        s_sim_rx_buffer,
        s_sim_rx_length);

    *received_length = s_sim_rx_length;

    s_sim_rx_length = 0u;
    s_sim_rx_pending = false;

    return GW_RESULT_OK;
}

/* -------------------------------------------------------------------------- */
/* Radio selection                                                            */
/* -------------------------------------------------------------------------- */

static GwRadioId_t gwTransportSelectRadio(void)
{
    if (s_config.radio == GW_TRANSPORT_RADIO_1)
    {
        return GW_RADIO_1;
    }

    /*
     * RADIO_0 and AUTO currently resolve to RADIO_0.
     *
     * AUTO policy will be expanded when the dual-radio
     * scheduling/arbitration layer is implemented.
     */
    return GW_RADIO_0;
}

/* -------------------------------------------------------------------------- */
/* Public API                                                                 */
/* -------------------------------------------------------------------------- */

GwResult_t gwTransportInit(
    const GwTransportConfig_t *config)
{
    if (s_initialized)
    {
        return GW_RESULT_ALREADY_INITIALIZED;
    }

    if (!gwTransportConfigValid(config))
    {
        return GW_RESULT_INVALID_ARG;
    }

    s_config = *config;

    s_sim_rx_length = 0u;
    s_sim_rx_pending = false;

    /*
     * SIM transport requires no hardware initialization.
     */
    if (s_config.type == GW_TRANSPORT_SIM)
    {
        s_initialized = true;
        return GW_RESULT_OK;
    }

    /*
     * RADIO transport requires the lower hardware layer.
     *
     * gwRadioInit()/gwRadioDeinit() remain owned by the
     * platform/core hardware layer.
     */
    if (s_config.type == GW_TRANSPORT_RADIO)
    {
        if (!gwRadioAllInitialized())
        {
            memset(
                &s_config,
                0,
                sizeof(s_config));

            return GW_RESULT_NOT_READY;
        }

        s_initialized = true;

        return GW_RESULT_OK;
    }

    memset(
        &s_config,
        0,
        sizeof(s_config));

    return GW_RESULT_ERROR;
}

GwResult_t gwTransportDeinit(void)
{
    if (!s_initialized)
    {
        return GW_RESULT_NOT_INITIALIZED;
    }

    /*
     * Transport does not own radio hardware lifetime.
     *
     * gwRadioInit()/gwRadioDeinit() remain owned by the
     * platform/core hardware layer.
     */
    memset(
        &s_config,
        0,
        sizeof(s_config));

    memset(
        s_sim_rx_buffer,
        0,
        sizeof(s_sim_rx_buffer));

    s_sim_rx_length = 0u;
    s_sim_rx_pending = false;
    s_initialized = false;

    return GW_RESULT_OK;
}

GwResult_t gwTransportSend(
    const uint8_t *data,
    size_t length)
{
    if (!s_initialized)
    {
        return GW_RESULT_NOT_INITIALIZED;
    }

    if (data == NULL || length == 0u)
    {
        return GW_RESULT_INVALID_ARG;
    }

    if (length > GW_TRANSPORT_SIM_BUFFER_SIZE)
    {
        return GW_RESULT_INVALID_ARG;
    }

    if (s_config.type == GW_TRANSPORT_SIM)
    {
        return gwTransportSimSend(
            data,
            length);
    }

    if (s_config.type == GW_TRANSPORT_RADIO)
    {
        const GwRadioId_t radio =
            gwTransportSelectRadio();

        return gwRadioSend(
            radio,
            data,
            length);
    }

    return GW_RESULT_ERROR;
}

GwResult_t gwTransportReceive(
    uint8_t *buffer,
    size_t buffer_size,
    size_t *received_length)
{
    if (!s_initialized)
    {
        return GW_RESULT_NOT_INITIALIZED;
    }

    if (buffer == NULL ||
        received_length == NULL ||
        buffer_size == 0u)
    {
        return GW_RESULT_INVALID_ARG;
    }

    *received_length = 0u;

    if (buffer_size > GW_TRANSPORT_SIM_BUFFER_SIZE)
    {
        buffer_size = GW_TRANSPORT_SIM_BUFFER_SIZE;
    }

    if (s_config.type == GW_TRANSPORT_SIM)
    {
        return gwTransportSimReceive(
            buffer,
            buffer_size,
            received_length);
    }

    if (s_config.type == GW_TRANSPORT_RADIO)
    {
        const GwRadioId_t radio =
            gwTransportSelectRadio();

        return gwRadioReceive(
            radio,
            buffer,
            buffer_size,
            received_length);
    }

    return GW_RESULT_ERROR;
}

GwResult_t gwTransportSetConfig(
    const GwTransportConfig_t *config)
{
    if (!s_initialized)
    {
        return GW_RESULT_NOT_INITIALIZED;
    }

    if (!gwTransportConfigValid(config))
    {
        return GW_RESULT_INVALID_ARG;
    }

    /*
     * Configuration changes while active are intentionally
     * rejected for this layer revision.
     *
     * This prevents changing the communication path while
     * runtime services may be actively using it.
     */
    if (config->type != s_config.type ||
        config->radio != s_config.radio)
    {
        return GW_RESULT_BUSY;
    }

    return GW_RESULT_OK;
}

GwResult_t gwTransportGetConfig(
    GwTransportConfig_t *config)
{
    if (!s_initialized)
    {
        return GW_RESULT_NOT_INITIALIZED;
    }

    if (config == NULL)
    {
        return GW_RESULT_INVALID_ARG;
    }

    *config = s_config;

    return GW_RESULT_OK;
}

bool gwTransportIsInitialized(void)
{
    return s_initialized;
}