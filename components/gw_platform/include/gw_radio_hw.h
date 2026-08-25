#ifndef GW_RADIO_HW_H
#define GW_RADIO_HW_H

/*
 * Orb Drive Main Gateway
 * ESP32-S3 dual SX1262 hardware definition.
 *
 * Final frozen LoRa pin map.
 *
 * Both radios share the same SPI bus.
 */

/* Shared SPI bus */
#define GW_RADIO_SPI_MOSI_GPIO       (35U)
#define GW_RADIO_SPI_MISO_GPIO       (37U)
#define GW_RADIO_SPI_SCK_GPIO        (36U)

/* Shared SPI host */
#define GW_RADIO_SPI_HOST            (2U)

/* Radio 0 — LoRa Channel A */
#define GW_RADIO0_CS_GPIO            (14U)
#define GW_RADIO0_DIO1_GPIO          (5U)
#define GW_RADIO0_RXEN_GPIO          (17U)
#define GW_RADIO0_TXEN_GPIO          (18U)
#define GW_RADIO0_RESET_GPIO         (39U)
#define GW_RADIO0_BUSY_GPIO          (21U)

/* Radio 1 — LoRa Channel B */
#define GW_RADIO1_CS_GPIO            (13U)
#define GW_RADIO1_DIO1_GPIO          (10U)
#define GW_RADIO1_RXEN_GPIO          (6U)
#define GW_RADIO1_TXEN_GPIO          (7U)
#define GW_RADIO1_RESET_GPIO         (11U)
#define GW_RADIO1_BUSY_GPIO          (12U)

/* Frozen LoRa channel frequencies */
#define GW_RADIO0_FREQUENCY_HZ       (865000000UL)
#define GW_RADIO1_FREQUENCY_HZ       (865500000UL)

#endif /* GW_RADIO_HW_H */