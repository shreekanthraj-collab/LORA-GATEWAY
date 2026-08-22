#ifndef GW_RADIO_H
#define GW_RADIO_H

#include "gw_types.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Production Gateway dual-radio interface.
 *
 * Hardware-specific SX1262, SPI, GPIO, and pin-map definitions
 * remain below this abstraction boundary.
 */

typedef enum
{
    GW_RADIO_0 = 0,
    GW_RADIO_1,
    GW_RADIO_COUNT
} GwRadioId_t;

typedef struct
{
    uint32_t frequency_hz;
    uint8_t bandwidth;
    uint8_t spreading_factor;
    uint8_t coding_rate;
    int8_t tx_power_dbm;
} GwRadioConfig_t;

GwResult_t gwRadioInit(const GwRadioConfig_t *config);

GwResult_t gwRadioDeinit(void);

GwResult_t gwRadioSend(GwRadioId_t radio,
                       const uint8_t *data,
                       size_t length);

GwResult_t gwRadioReceive(GwRadioId_t radio,
                          uint8_t *buffer,
                          size_t buffer_size,
                          size_t *received_length);

bool gwRadioIsInitialized(GwRadioId_t radio);

bool gwRadioAllInitialized(void);

GwResult_t gwRadioGetConfig(GwRadioConfig_t *config);

#ifdef __cplusplus
}
#endif

#endif /* GW_RADIO_H */