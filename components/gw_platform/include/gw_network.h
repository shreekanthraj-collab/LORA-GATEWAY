#ifndef GW_NETWORK_H
#define GW_NETWORK_H

#include "gw_types.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Production Gateway network interface.
 *
 * Network-specific implementation must remain below
 * this abstraction boundary.
 *
 * No Wi-Fi, Ethernet, MQTT, TLS, or socket definitions
 * belong here.
 */

GwResult_t gwNetworkInit(void);

GwResult_t gwNetworkDeinit(void);

GwResult_t gwNetworkConnect(void);

GwResult_t gwNetworkDisconnect(void);

GwResult_t gwNetworkSend(const uint8_t *data, size_t length);

GwResult_t gwNetworkReceive(uint8_t *buffer,
                            size_t buffer_size,
                            size_t *received_length);

bool gwNetworkIsInitialized(void);

bool gwNetworkIsConnected(void);

#ifdef __cplusplus
}
#endif

#endif /* GW_NETWORK_H */
