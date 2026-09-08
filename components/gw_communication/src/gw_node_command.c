#include "gw_node_command.h"

#include <stddef.h>

GwResult_t gwNodeCommandBuild(
    uint32_t message_id,
    uint32_t timestamp,
    uint32_t node_id,
    uint8_t opcode,
    const uint8_t *payload,
    uint16_t payload_length,
    uint8_t *buffer,
    size_t buffer_capacity,
    size_t *encoded_length)
{
    GwProtocolFrame_t frame = {0};

    if (buffer == NULL ||
        encoded_length == NULL)
    {
        return GW_RESULT_INVALID_ARG;
    }

    if (payload_length > 0u && payload == NULL)
    {
        return GW_RESULT_INVALID_ARG;
    }

    if (!gwProtocolIsCommandValid(opcode))
    {
        return GW_RESULT_INVALID_ARG;
    }

    frame.version = GW_PROTOCOL_VERSION;
    frame.packet_type = GW_PKT_CMD;
    frame.message_id = message_id;
    frame.timestamp = timestamp;
    frame.node_id = node_id;
    frame.payload = payload;
    frame.payload_length = payload_length;

    return gwProtocolEncode(
        &frame,
        buffer,
        buffer_capacity,
        encoded_length);
}
