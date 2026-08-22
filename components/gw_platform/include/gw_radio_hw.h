#ifndef GW_RADIO_HW_H
#define GW_RADIO_HW_H

/*
 * Gateway dual-SX1262 temporary hardware definition.
 *
 * WARNING:
 * These GPIO assignments are PLACEHOLDERS for board commencement.
 * Replace them when the final Gateway board pin map is available.
 *
 * MOSI/MISO/SCK are shared by both radios.
 */

/* Shared SPI bus */
#define GW_RADIO_SPI_MOSI_GPIO       (11U)
#define GW_RADIO_SPI_MISO_GPIO       (13U)
#define GW_RADIO_SPI_SCK_GPIO        (12U)

/* Shared SPI host */
#define GW_RADIO_SPI_HOST             (2U)

/* Radio 0 */
#define GW_RADIO0_CS_GPIO             (14U)
#define GW_RADIO0_DIO1_GPIO           (5U)
#define GW_RADIO0_RXEN_GPIO           (17U)
#define GW_RADIO0_TXEN_GPIO           (18U)
#define GW_RADIO0_RESET_GPIO          (38U)
#define GW_RADIO0_BUSY_GPIO           (47U)

/* Radio 1 */
#define GW_RADIO1_CS_GPIO             (15U)
#define GW_RADIO1_DIO1_GPIO           (6U)
#define GW_RADIO1_RXEN_GPIO           (19U)
#define GW_RADIO1_TXEN_GPIO           (20U)
#define GW_RADIO1_RESET_GPIO          (39U)
#define GW_RADIO1_BUSY_GPIO           (48U)

/* Temporary common LoRa configuration */
#define GW_RADIO_DEFAULT_FREQUENCY_HZ       (868000000UL)
#define GW_RADIO_DEFAULT_BANDWIDTH           (0U)
#define GW_RADIO_DEFAULT_SPREADING_FACTOR    (7U)
#define GW_RADIO_DEFAULT_CODING_RATE         (1U)
#define GW_RADIO_DEFAULT_TX_POWER_DBM        (17)

#endif /* GW_RADIO_HW_H */