#ifndef GW_COMMUNICATION_H
#define GW_COMMUNICATION_H

#include "gw_types.h"
#include "gw_transport.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GW_COMM_MAX_PACKET_SIZE 256u

GwResult_t gwCommunicationInit(
    const GwTransportConfig_t *config);

GwResult_t gwCommunicationDeinit(void);

GwResult_t gwCommunicationSend(
    const uint8_t *packet,
    size_t length);

GwResult_t gwCommunicationReceive(
    uint8_t *buffer,
    size_t buffer_size,
    size_t *received_length);

GwResult_t gwCommunicationSetTransport(
    const GwTransportConfig_t *config);

GwResult_t gwCommunicationGetTransport(
    GwTransportConfig_t *config);

bool gwCommunicationIsInitialized(void);

#ifdef __cplusplus
}
#endif

#endif /* GW_COMMUNICATION_H */