#ifndef GW_PROTOCOL_CODEC_H
#define GW_PROTOCOL_CODEC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "gw_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GW_PROTOCOL_VERSION          (1U)
#define GW_PROTOCOL_MAX_PAYLOAD      (192U)
#define GW_PROTOCOL_HEADER_SIZE      (14U)
#define GW_PROTOCOL_CRC_SIZE         (2U)
#define GW_PROTOCOL_MAX_FRAME        (GW_PROTOCOL_HEADER_SIZE + GW_PROTOCOL_MAX_PAYLOAD + GW_PROTOCOL_CRC_SIZE)

/* Packet types */
#define GW_PKT_COMMAND               (0x10U)
#define GW_PKT_ACK                   (0xAAU)
#define GW_PKT_STATUS                (0x20U)
#define GW_PKT_EVENT                 (0x30U)
#define GW_PKT_SCHED                 (0x40U)
#define GW_PKT_EOL_START             (0x60U)
#define GW_PKT_EOL_RESULT            (0x61U)
#define GW_PKT_EOL_SUMMARY           (0x62U)

/* Command opcodes */
#define GW_CMD_OPEN                  (0x01U)
#define GW_CMD_CLOSE                 (0x02U)
#define GW_CMD_STOP                  (0x03U)
#define GW_CMD_GET_STATUS            (0x04U)
#define GW_CMD_SET_POSITION          (0x05U)
#define GW_CMD_SET_SCHEDULE          (0x06U)
#define GW_CMD_CLEAR_SCHEDULE        (0x07U)
#define GW_CMD_SET_CURRENT           (0x0EU)
#define GW_CMD_CALIBRATE             (0x0FU)
#define GW_CMD_CAL_SET               (0x10U)
#define GW_CMD_CAL_ABORT             (0x11U)
#define GW_CMD_ENTER_EOL             (0x12U)
#define GW_CMD_REBIND_OWNER          (0x13U)
#define GW_CMD_SET_DISENGAGE_CURRENT (0x15U)

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
    GwProtocolFrame_t *frame);

bool gwProtocolIsPacketTypeValid(
    uint8_t packet_type);

bool gwProtocolIsCommandValid(
    uint8_t opcode);

uint16_t gwProtocolCrc16(
    const uint8_t *data,
    size_t length);

#ifdef __cplusplus
}
#endif

#endif /* GW_PROTOCOL_CODEC_H */
