#ifndef GW_RADIO_H
#define GW_RADIO_H

#include "gw_types.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Production Gateway radio interface.
 *
 * Hardware-specific radio implementation must remain
 * below this abstraction boundary.
 *
 * No SX1262, SPI, GPIO, or pin-map definitions belong here.
 */

GwResult_t gwRadioInit(void);

GwResult_t gwRadioDeinit(void);

GwResult_t gwRadioSend(const uint8_t *data, size_t length);

GwResult_t gwRadioReceive(uint8_t *buffer,
						  size_t buffer_size,
						  size_t *received_length);

bool gwRadioIsInitialized(void);

#ifdef __cplusplus
}
#endif
#endif /* GW_RADIO_H */