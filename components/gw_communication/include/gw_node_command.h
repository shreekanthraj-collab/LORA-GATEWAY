#ifndef GW_NODE_COMMAND_H
#define GW_NODE_COMMAND_H

#include <stddef.h>
#include <stdint.h>

#include "gw_protocol_codec.h"
#include "gw_types.h"

#ifdef __cplusplus
extern "C" {
#endif

GwResult_t gwNodeCommandBuild(
    uint32_t message_id,
    uint32_t timestamp,
    uint32_t node_id,
    uint8_t opcode,
    const uint8_t *payload,
    uint16_t payload_length,
    uint8_t *buffer,
    size_t buffer_capacity,
    size_t *encoded_length);

#ifdef __cplusplus
}
#endif

#endif /* GW_NODE_COMMAND_H */
