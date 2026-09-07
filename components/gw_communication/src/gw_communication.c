#include "gw_communication.h"

#include <stddef.h>

static bool s_initialized = false;

GwResult_t gwCommunicationInit(
    const GwTransportConfig_t *config)
{
    GwResult_t result;

    if (s_initialized)
    {
        return GW_RESULT_ALREADY_INITIALIZED;
    }

    if (config == NULL)
    {
        return GW_RESULT_INVALID_ARG;
    }

    result = gwTransportInit(config);

    if (result != GW_RESULT_OK)
    {
        return result;
    }

    s_initialized = true;

    return GW_RESULT_OK;
}

GwResult_t gwCommunicationDeinit(void)
{
    GwResult_t result;

    if (!s_initialized)
    {
        return GW_RESULT_NOT_INITIALIZED;
    }

    result = gwTransportDeinit();

    if (result != GW_RESULT_OK)
    {
        return result;
    }

    s_initialized = false;

    return GW_RESULT_OK;
}

bool gwCommunicationIsInitialized(void)
{
    return s_initialized;
}

GwResult_t gwCommunicationSend(
    const uint8_t *packet,
    size_t length)
{
    if (!s_initialized)
    {
        return GW_RESULT_NOT_INITIALIZED;
    }

    if (packet == NULL || length == 0u)
    {
        return GW_RESULT_INVALID_ARG;
    }

    if (length > GW_COMM_MAX_PACKET_SIZE)
    {
        return GW_RESULT_INVALID_ARG;
    }

    return gwTransportSend(
        packet,
        length);
}

GwResult_t gwCommunicationReceive(
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

    if (buffer_size > GW_COMM_MAX_PACKET_SIZE)
    {
        buffer_size = GW_COMM_MAX_PACKET_SIZE;
    }

    return gwTransportReceive(
        buffer,
        buffer_size,
        received_length);
}

GwResult_t gwCommunicationSendFrame(
    const GwProtocolFrame_t *frame)
{
    uint8_t buffer[GW_PROTOCOL_MAX_FRAME];
    size_t encoded_length = 0u;
    GwResult_t result;

    if (!s_initialized)
    {
        return GW_RESULT_NOT_INITIALIZED;
    }

    if (frame == NULL)
    {
        return GW_RESULT_INVALID_ARG;
    }

    result = gwProtocolEncode(
        frame,
        buffer,
        sizeof(buffer),
        &encoded_length);

    if (result != GW_RESULT_OK)
    {
        return result;
    }

    return gwTransportSend(
        buffer,
        encoded_length);
}

GwResult_t gwCommunicationReceiveFrame(
    GwProtocolFrame_t *frame,
    uint8_t *payload_buffer,
    size_t payload_buffer_size)
{
    uint8_t buffer[GW_PROTOCOL_MAX_FRAME];
    size_t received_length = 0u;
    GwResult_t result;

    if (!s_initialized)
    {
        return GW_RESULT_NOT_INITIALIZED;
    }

    if (frame == NULL ||
        payload_buffer == NULL ||
        payload_buffer_size == 0u)
    {
        return GW_RESULT_INVALID_ARG;
    }

    result = gwTransportReceive(
        buffer,
        sizeof(buffer),
        &received_length);

    if (result != GW_RESULT_OK)
    {
        return result;
    }

    return gwProtocolDecode(
        buffer,
        received_length,
        frame,
        payload_buffer,
        payload_buffer_size);
}

GwResult_t gwCommunicationSetTransport(
    const GwTransportConfig_t *config)
{
    if (!s_initialized)
    {
        return GW_RESULT_NOT_INITIALIZED;
    }

    if (config == NULL)
    {
        return GW_RESULT_INVALID_ARG;
    }

    return gwTransportSetConfig(config);
}

GwResult_t gwCommunicationGetTransport(
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

    return gwTransportGetConfig(config);
}
