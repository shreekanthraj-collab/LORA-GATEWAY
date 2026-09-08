#ifndef GW_PROTOCOL_CODEC_H
#define GW_PROTOCOL_CODEC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "gw_protocol.h"
#include "gw_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GW_PROTOCOL_VERSION     (1U)
#define GW_PROTOCOL_CRC_SIZE    (2U)
#define GW_PROTOCOL_MAX_PAYLOAD (192U)
#define GW_PROTOCOL_HEADER_SIZE (14U)
#define GW_PROTOCOL_MAX_FRAME   (GW_PROTOCOL_HEADER_SIZE + GW_PROTOCOL_MAX_PAYLOAD + GW_PROTOCOL_CRC_SIZE)

typedef struct
{
    uint8_t version;
    uint8_t packet_type;
    uint32_t message_id;
    uint32_t timestamp;
    uint32_t node_id;
    const uint8_t *payload;
    uint16_t payload_length;
} GwProtocolFrame_t;

GwResult_t gwProtocolEncode(
    const GwProtocolFrame_t *frame,
    uint8_t *buffer,
    size_t capacity,
    size_t *encoded_length);

GwResult_t gwProtocolDecode(
    const uint8_t *buffer,
    size_t length,
    GwProtocolFrame_t *frame,
    uint8_t *payload_buffer,
    size_t payload_buffer_size);

uint16_t gwProtocolCrc16(
    const uint8_t *data,
    size_t length);

bool gwProtocolIsPacketTypeValid(
    uint8_t packet_type);

bool gwProtocolIsCommandValid(
    uint8_t opcode);

#ifdef __cplusplus
}
#endif

#endif /* GW_PROTOCOL_CODEC_H */
