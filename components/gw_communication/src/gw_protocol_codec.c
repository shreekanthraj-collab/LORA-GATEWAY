#include "gw_protocol_codec.h"

#include <string.h>

static void gwPutU16(
    uint8_t *p,
    uint16_t value)
{
    p[0] = (uint8_t)(value >> 8U);
    p[1] = (uint8_t)value;
}

static void gwPutU32(
    uint8_t *p,
    uint32_t value)
{
    p[0] = (uint8_t)(value >> 24U);
    p[1] = (uint8_t)(value >> 16U);
    p[2] = (uint8_t)(value >> 8U);
    p[3] = (uint8_t)value;
}

static uint16_t gwGetU16(
    const uint8_t *p)
{
    return (uint16_t)(((uint16_t)p[0] << 8U) |
                      (uint16_t)p[1]);
}

static uint32_t gwGetU32(
    const uint8_t *p)
{
    return ((uint32_t)p[0] << 24U) |
           ((uint32_t)p[1] << 16U) |
           ((uint32_t)p[2] << 8U) |
           (uint32_t)p[3];
}

uint16_t gwProtocolCrc16(
    const uint8_t *data,
    size_t length)
{
    uint16_t crc = 0xFFFFU;

    if (data == NULL)
    {
        return 0U;
    }

    for (size_t i = 0U; i < length; ++i)
    {
        crc ^= (uint16_t)data[i];

        for (uint8_t bit = 0U; bit < 8U; ++bit)
        {
            if ((crc & 0x0001U) != 0U)
            {
                crc = (uint16_t)((crc >> 1U) ^ 0xA001U);
            }
            else
            {
                crc >>= 1U;
            }
        }
    }

    return crc;
}

bool gwProtocolIsPacketTypeValid(
    uint8_t packet_type)
{
    switch (packet_type)
    {
        case GW_PKT_COMMAND:
        case GW_PKT_ACK:
        case GW_PKT_STATUS:
        case GW_PKT_EVENT:
        case GW_PKT_SCHED:
        case GW_PKT_EOL_START:
        case GW_PKT_EOL_RESULT:
        case GW_PKT_EOL_SUMMARY:
            return true;

        default:
            return false;
    }
}

bool gwProtocolIsCommandValid(
    uint8_t opcode)
{
    switch (opcode)
    {
        case GW_CMD_OPEN:
        case GW_CMD_CLOSE:
        case GW_CMD_STOP:
        case GW_CMD_GET_STATUS:
        case GW_CMD_SET_POSITION:
        case GW_CMD_SET_SCHEDULE:
        case GW_CMD_CLEAR_SCHEDULE:
        case GW_CMD_SET_CURRENT:
        case GW_CMD_CALIBRATE:
        case GW_CMD_CAL_SET:
        case GW_CMD_CAL_ABORT:
        case GW_CMD_ENTER_EOL:
        case GW_CMD_REBIND_OWNER:
        case GW_CMD_SET_DISENGAGE_CURRENT:
            return true;

        default:
            return false;
    }
}

GwResult_t gwProtocolEncode(
    const GwProtocolFrame_t *frame,
    uint8_t *buffer,
    size_t capacity,
    size_t *encoded_length)
{
    if (frame == NULL ||
        buffer == NULL ||
        encoded_length == NULL)
    {
        return GW_RESULT_INVALID_ARG;
    }

    *encoded_length = 0U;

    if (frame->version != GW_PROTOCOL_VERSION)
    {
        return GW_RESULT_INVALID_ARG;
    }

    if (!gwProtocolIsPacketTypeValid(frame->packet_type))
    {
        return GW_RESULT_INVALID_ARG;
    }

    if (frame->payload_length > GW_PROTOCOL_MAX_PAYLOAD)
    {
        return GW_RESULT_INVALID_ARG;
    }

    if (frame->payload_length > 0U &&
        frame->payload == NULL)
    {
        return GW_RESULT_INVALID_ARG;
    }

    const size_t total_length =
        GW_PROTOCOL_HEADER_SIZE +
        (size_t)frame->payload_length +
        GW_PROTOCOL_CRC_SIZE;

    if (capacity < total_length)
    {
        return GW_RESULT_INVALID_ARG;
    }

    buffer[0] = frame->version;
    buffer[1] = frame->packet_type;

    gwPutU32(
        &buffer[2],
        frame->message_id);

    gwPutU32(
        &buffer[6],
        frame->timestamp);

    gwPutU32(
        &buffer[10],
        frame->node_id);

    if (frame->payload_length > 0U)
    {
        memcpy(
            &buffer[GW_PROTOCOL_HEADER_SIZE],
            frame->payload,
            frame->payload_length);
    }

    const uint16_t crc =
        gwProtocolCrc16(
            buffer,
            GW_PROTOCOL_HEADER_SIZE +
                frame->payload_length);

    gwPutU16(
        &buffer[GW_PROTOCOL_HEADER_SIZE +
                frame->payload_length],
        crc);

    *encoded_length = total_length;

    return GW_RESULT_OK;
}

GwResult_t gwProtocolDecode(
    const uint8_t *buffer,
    size_t length,
    GwProtocolFrame_t *frame)
{
    if (buffer == NULL ||
        frame == NULL)
    {
        return GW_RESULT_INVALID_ARG;
    }

    if (length < GW_PROTOCOL_HEADER_SIZE +
                 GW_PROTOCOL_CRC_SIZE)
    {
        return GW_RESULT_INVALID_ARG;
    }

    if (buffer[0] != GW_PROTOCOL_VERSION)
    {
        return GW_RESULT_INVALID_ARG;
    }

    if (!gwProtocolIsPacketTypeValid(buffer[1]))
    {
        return GW_RESULT_INVALID_ARG;
    }

    const size_t payload_length =
        length -
        GW_PROTOCOL_HEADER_SIZE -
        GW_PROTOCOL_CRC_SIZE;

    if (payload_length > GW_PROTOCOL_MAX_PAYLOAD)
    {
        return GW_RESULT_INVALID_ARG;
    }

    const uint16_t received_crc =
        gwGetU16(
            &buffer[length - GW_PROTOCOL_CRC_SIZE]);

    const uint16_t calculated_crc =
        gwProtocolCrc16(
            buffer,
            length - GW_PROTOCOL_CRC_SIZE);

    if (received_crc != calculated_crc)
    {
        return GW_RESULT_ERROR;
    }

    frame->version = buffer[0];
    frame->packet_type = buffer[1];

    frame->message_id =
        gwGetU32(&buffer[2]);

    frame->timestamp =
        gwGetU32(&buffer[6]);

    frame->node_id =
        gwGetU32(&buffer[10]);

    frame->payload =
        &buffer[GW_PROTOCOL_HEADER_SIZE];

    frame->payload_length =
        (uint16_t)payload_length;

    return GW_RESULT_OK;
}
