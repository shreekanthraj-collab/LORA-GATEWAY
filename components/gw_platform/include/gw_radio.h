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
 * remain inside the Gateway platform implementation.
 *
 * EOL/manufacturing diagnostics use the public diagnostic APIs
 * exposed here rather than accessing GPIO/SPI directly.
 */

/* ========================================================================== */
/* Radio identity                                                              */
/* ========================================================================== */

typedef enum
{
    GW_RADIO_0 = 0,
    GW_RADIO_1,
    GW_RADIO_COUNT
} GwRadioId_t;

/* ========================================================================== */
/* Radio configuration                                                         */
/* ========================================================================== */

typedef struct
{
    uint32_t frequency_hz;
    uint8_t bandwidth;
    uint8_t spreading_factor;
    uint8_t coding_rate;
    int8_t tx_power_dbm;
} GwRadioConfig_t;

/* ========================================================================== */
/* Lifecycle                                                                   */
/* ========================================================================== */

/**
 * Initialize the shared SPI bus and both SX1262 radios.
 */
GwResult_t gwRadioInit(
    const GwRadioConfig_t *config);

/**
 * Deinitialize both radios and release the shared SPI bus.
 */
GwResult_t gwRadioDeinit(void);

/**
 * Check whether a specific radio is initialized.
 */
bool gwRadioIsInitialized(
    GwRadioId_t radio);

/**
 * Check whether all Gateway radios are initialized.
 */
bool gwRadioAllInitialized(void);

/**
 * Get the currently configured radio parameters.
 */
GwResult_t gwRadioGetConfig(
    GwRadioConfig_t *config);

/* ========================================================================== */
/* Radio data path                                                             */
/* ========================================================================== */

/**
 * Transmit data through the selected radio.
 *
 * Current production implementation may return
 * GW_RESULT_NOT_READY until the SX1262 TX path is implemented.
 */
GwResult_t gwRadioSend(
    GwRadioId_t radio,
    const uint8_t *data,
    size_t length);

/**
 * Receive data from the selected radio.
 *
 * Current production implementation may return
 * GW_RESULT_NOT_READY until the SX1262 RX path is implemented.
 */
GwResult_t gwRadioReceive(
    GwRadioId_t radio,
    uint8_t *buffer,
    size_t buffer_size,
    size_t *received_length);

/* ========================================================================== */
/* Hardware diagnostics                                                        */
/* ========================================================================== */

/**
 * Perform a hardware reset of the selected SX1262.
 *
 * Used by Gateway EOL diagnostics.
 */
GwResult_t gwRadioReset(
    GwRadioId_t radio);

/**
 * Read the SX1262 BUSY GPIO state.
 *
 * busy = true  -> BUSY GPIO is HIGH
 * busy = false -> BUSY GPIO is LOW
 */
GwResult_t gwRadioGetBusy(
    GwRadioId_t radio,
    bool *busy);

/**
 * Read the SX1262 DIO1 GPIO state.
 *
 * dio1 = true  -> DIO1 GPIO is HIGH
 * dio1 = false -> DIO1 GPIO is LOW
 */
GwResult_t gwRadioGetDio1(
    GwRadioId_t radio,
    bool *dio1);

/**
 * Control the external RXEN/TXEN RF switch signals.
 *
 * Valid states:
 *
 *   rx_enable = false, tx_enable = false
 *       Both disabled
 *
 *   rx_enable = true,  tx_enable = false
 *       RX enabled
 *
 *   rx_enable = false, tx_enable = true
 *       TX enabled
 *
 * RX and TX must not be enabled simultaneously.
 */
GwResult_t gwRadioSetRxTx(
    GwRadioId_t radio,
    bool rx_enable,
    bool tx_enable);

#ifdef __cplusplus
}
#endif

#endif /* GW_RADIO_H */