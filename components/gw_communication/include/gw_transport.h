#ifndef GW_TRANSPORT_H
#define GW_TRANSPORT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "gw_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    GW_TRANSPORT_SIM = 0,
    GW_TRANSPORT_RADIO,
    GW_TRANSPORT_COUNT
} GwTransportType_t;

typedef enum
{
    GW_TRANSPORT_RADIO_0 = 0,
    GW_TRANSPORT_RADIO_1,
    GW_TRANSPORT_RADIO_AUTO
} GwTransportRadio_t;

typedef struct
{
    GwTransportType_t type;
    GwTransportRadio_t radio;
} GwTransportConfig_t;

GwResult_t gwTransportInit(
    const GwTransportConfig_t *config);

GwResult_t gwTransportDeinit(void);

GwResult_t gwTransportSend(
    const uint8_t *data,
    size_t length);

GwResult_t gwTransportReceive(
    uint8_t *buffer,
    size_t buffer_size,
    size_t *received_length);

GwResult_t gwTransportSetConfig(
    const GwTransportConfig_t *config);

GwResult_t gwTransportGetConfig(
    GwTransportConfig_t *config);

bool gwTransportIsInitialized(void);

#ifdef __cplusplus
}
#endif

#endif /* GW_TRANSPORT_H */
