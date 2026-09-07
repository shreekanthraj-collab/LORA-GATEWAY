#ifndef GW_COMMUNICATION_H
#define GW_COMMUNICATION_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "gw_transport.h"
#include "gw_protocol_codec.h"
#include "gw_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GW_COMM_MAX_PACKET_SIZE \
    (GW_PROTOCOL_MAX_FRAME)

/* -------------------------------------------------------------------------- */
/* Lifecycle                                                                  */
/* -------------------------------------------------------------------------- */

GwResult_t gwCommunicationInit(
    const GwTransportConfig_t *config);

GwResult_t gwCommunicationDeinit(void);

bool gwCommunicationIsInitialized(void);

/* -------------------------------------------------------------------------- */
/* Raw transport                                                              */
/* -------------------------------------------------------------------------- */

GwResult_t gwCommunicationSend(
    const uint8_t *packet,
    size_t length);

GwResult_t gwCommunicationReceive(
    uint8_t *buffer,
    size_t buffer_size,
    size_t *received_length);

/* -------------------------------------------------------------------------- */
/* Transport configuration                                                     */
/* -------------------------------------------------------------------------- */

GwResult_t gwCommunicationSetTransport(
    const GwTransportConfig_t *config);

GwResult_t gwCommunicationGetTransport(
    GwTransportConfig_t *config);

#ifdef __cplusplus
}
#endif

#endif /* GW_COMMUNICATION_H */
