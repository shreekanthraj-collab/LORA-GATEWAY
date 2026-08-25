#include "gw_command_service.h"

#include "gw_config.h"
#include "gw_protocol.h"
#include "gw_transport.h"

#include <stddef.h>

static bool s_initialized = false;
static uint8_t s_sequence = 0u;

static uint8_t gwCommandNextSequence(void)
{
    const uint8_t sequence = s_sequence;
    s_sequence++;

    return sequence;
}

GwResult_t gwCommandServiceInit(void)
{
    if (s_initialized)
    {
        return GW_RESULT_ALREADY_INITIALIZED;
    }

    s_sequence = 0u;
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
    s_sequence = 0u;

    return GW_RESULT_OK;
}

GwResult_t gwCommandServiceSend(
    const GwCommandRequest_t *request)
{
    GwLoRaCmd_t packet;

    if (!s_initialized)
    {
        return GW_RESULT_NOT_INITIALIZED;
    }

    if (request == NULL)
    {
        return GW_RESULT_INVALID_ARG;
    }

    packet.node = request->node;
    packet.type = GW_PKT_CMD;
    packet.cmd = request->command;
    packet.value = request->value;
    packet.seq = gwCommandNextSequence();
    packet.gwid = GW_CONFIG_GWID_UNBOUND;

    packet.crc8 = gwPacketCrc8(
        (const uint8_t *)&packet,
        sizeof(packet) - 1u);

    return gwTransportSend(
        (const uint8_t *)&packet,
        sizeof(packet));
}

bool gwCommandServiceIsInitialized(void)
{
    return s_initialized;
}