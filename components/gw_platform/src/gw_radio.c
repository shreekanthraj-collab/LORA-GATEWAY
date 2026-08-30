#include "gw_radio.h"
#include "gw_radio_hw.h"

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_err.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <string.h>

/* -------------------------------------------------------------------------- */
/* SX1262 commands                                                            */
/* -------------------------------------------------------------------------- */

#define SX1262_CMD_SET_STANDBY            (0x80U)
#define SX1262_CMD_SET_RF_FREQUENCY       (0x86U)
#define SX1262_CMD_SET_PACKET_TYPE        (0x8AU)
#define SX1262_CMD_SET_MODULATION_PARAMS  (0x8BU)
#define SX1262_CMD_SET_PACKET_PARAMS      (0x8CU)
#define SX1262_CMD_SET_TX_PARAMS          (0x8EU)
#define SX1262_CMD_SET_BUFFER_BASE_ADDR   (0x8FU)
#define SX1262_CMD_SET_TX                  (0x83U)
#define SX1262_CMD_SET_RX                  (0x82U)

#define SX1262_CMD_GET_STATUS             (0xC0U)
#define SX1262_CMD_GET_IRQ_STATUS         (0x12U)
#define SX1262_CMD_CLEAR_IRQ_STATUS       (0x02U)
#define SX1262_CMD_GET_RX_BUFFER_STATUS   (0x13U)
#define SX1262_CMD_READ_BUFFER            (0x1EU)
#define SX1262_CMD_WRITE_BUFFER           (0x0EU)

#define SX1262_PACKET_TYPE_LORA           (0x01U)

#define SX1262_STANDBY_RC                (0x00U)

#define SX1262_RESET_LOW_MS              (2U)
#define SX1262_RESET_HIGH_WAIT_MS        (10U)

#define SX1262_SPI_TIMEOUT_MS            (100U)

#define SX1262_BUFFER_BASE_TX            (0x00U)
#define SX1262_BUFFER_BASE_RX            (0x00U)

#define SX1262_TX_TIMEOUT_INFINITE       (0xFFFFFFU)
#define SX1262_RX_TIMEOUT_SINGLE         (0x000000U)

/* -------------------------------------------------------------------------- */
/* Internal state                                                             */
/* -------------------------------------------------------------------------- */

static bool s_radio_initialized[GW_RADIO_COUNT] = {
    false,
    false
};

static GwRadioConfig_t s_radio_config;

static spi_device_handle_t s_radio_handle[GW_RADIO_COUNT] = {
    NULL,
    NULL
};

static bool s_spi_bus_initialized = false;

/* -------------------------------------------------------------------------- */
/* Internal helpers                                                           */
/* -------------------------------------------------------------------------- */

static bool gwRadioIdValid(
    GwRadioId_t radio)
{
    return radio < GW_RADIO_COUNT;
}

static gpio_num_t gwRadioResetGpio(
    GwRadioId_t radio)
{
    if (radio == GW_RADIO_0)
    {
        return GW_RADIO0_RESET_GPIO;
    }

    return GW_RADIO1_RESET_GPIO;
}

static gpio_num_t gwRadioBusyGpio(
    GwRadioId_t radio)
{
    if (radio == GW_RADIO_0)
    {
        return GW_RADIO0_BUSY_GPIO;
    }

    return GW_RADIO1_BUSY_GPIO;
}

static gpio_num_t gwRadioDio1Gpio(
    GwRadioId_t radio)
{
    if (radio == GW_RADIO_0)
    {
        return GW_RADIO0_DIO1_GPIO;
    }

    return GW_RADIO1_DIO1_GPIO;
}

static gpio_num_t gwRadioRxenGpio(
    GwRadioId_t radio)
{
    if (radio == GW_RADIO_0)
    {
        return GW_RADIO0_RXEN_GPIO;
    }

    return GW_RADIO1_RXEN_GPIO;
}

static gpio_num_t gwRadioTxenGpio(
    GwRadioId_t radio)
{
    if (radio == GW_RADIO_0)
    {
        return GW_RADIO0_TXEN_GPIO;
    }

    return GW_RADIO1_TXEN_GPIO;
}

/* -------------------------------------------------------------------------- */
/* GPIO                                                                       */
/* -------------------------------------------------------------------------- */

static esp_err_t gwRadioConfigureGpios(void)
{
    const gpio_config_t output_config = {
        .pin_bit_mask =
            (1ULL << GW_RADIO0_RESET_GPIO) |
            (1ULL << GW_RADIO0_RXEN_GPIO) |
            (1ULL << GW_RADIO0_TXEN_GPIO) |
            (1ULL << GW_RADIO1_RESET_GPIO) |
            (1ULL << GW_RADIO1_RXEN_GPIO) |
            (1ULL << GW_RADIO1_TXEN_GPIO),

        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    esp_err_t err = gpio_config(
        &output_config);

    if (err != ESP_OK)
    {
        return err;
    }

    const gpio_config_t input_config = {
        .pin_bit_mask =
            (1ULL << GW_RADIO0_BUSY_GPIO) |
            (1ULL << GW_RADIO0_DIO1_GPIO) |
            (1ULL << GW_RADIO1_BUSY_GPIO) |
            (1ULL << GW_RADIO1_DIO1_GPIO),

        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    err = gpio_config(
        &input_config);

    if (err != ESP_OK)
    {
        return err;
    }

    /*
     * Safe initial RF control state.
     *
     * RESET inactive.
     * RXEN/TXEN disabled.
     */
    gpio_set_level(
        GW_RADIO0_RESET_GPIO,
        1);

    gpio_set_level(
        GW_RADIO1_RESET_GPIO,
        1);

    gpio_set_level(
        GW_RADIO0_RXEN_GPIO,
        0);

    gpio_set_level(
        GW_RADIO0_TXEN_GPIO,
        0);

    gpio_set_level(
        GW_RADIO1_RXEN_GPIO,
        0);

    gpio_set_level(
        GW_RADIO1_TXEN_GPIO,
        0);

    return ESP_OK;
}

/* -------------------------------------------------------------------------- */
/* SPI                                                                        */
/* -------------------------------------------------------------------------- */

static esp_err_t gwRadioConfigureSpi(void)
{
    const spi_bus_config_t bus_config = {
        .mosi_io_num = GW_RADIO_SPI_MOSI_GPIO,
        .miso_io_num = GW_RADIO_SPI_MISO_GPIO,
        .sclk_io_num = GW_RADIO_SPI_SCK_GPIO,

        .quadwp_io_num = -1,
        .quadhd_io_num = -1,

        .data4_io_num = -1,
        .data5_io_num = -1,
        .data6_io_num = -1,
        .data7_io_num = -1,

        .max_transfer_sz = 256
    };

    esp_err_t err = spi_bus_initialize(
        SPI2_HOST,
        &bus_config,
        SPI_DMA_DISABLED);

    if (err != ESP_OK)
    {
        return err;
    }

    s_spi_bus_initialized = true;

    const spi_device_interface_config_t radio0_config = {
        .clock_speed_hz = 8000000,
        .mode = 0,
        .spics_io_num = GW_RADIO0_CS_GPIO,
        .queue_size = 1
    };

    err = spi_bus_add_device(
        SPI2_HOST,
        &radio0_config,
        &s_radio_handle[GW_RADIO_0]);

    if (err != ESP_OK)
    {
        return err;
    }

    const spi_device_interface_config_t radio1_config = {
        .clock_speed_hz = 8000000,
        .mode = 0,
        .spics_io_num = GW_RADIO1_CS_GPIO,
        .queue_size = 1
    };

    err = spi_bus_add_device(
        SPI2_HOST,
        &radio1_config,
        &s_radio_handle[GW_RADIO_1]);

    if (err != ESP_OK)
    {
        return err;
    }

    return ESP_OK;
}

static void gwRadioCleanupSpi(void)
{
    if (s_radio_handle[GW_RADIO_1] != NULL)
    {
        spi_bus_remove_device(
            s_radio_handle[GW_RADIO_1]);

        s_radio_handle[GW_RADIO_1] = NULL;
    }

    if (s_radio_handle[GW_RADIO_0] != NULL)
    {
        spi_bus_remove_device(
            s_radio_handle[GW_RADIO_0]);

        s_radio_handle[GW_RADIO_0] = NULL;
    }

    if (s_spi_bus_initialized)
    {
        spi_bus_free(SPI2_HOST);

        s_spi_bus_initialized = false;
    }
}

/* -------------------------------------------------------------------------- */
/* SX1262 BUSY handling                                                       */
/* -------------------------------------------------------------------------- */

static esp_err_t gwRadioWaitWhileBusy(
    GwRadioId_t radio)
{
    if (!gwRadioIdValid(radio))
    {
        return ESP_ERR_INVALID_ARG;
    }

    const gpio_num_t busy_gpio =
        gwRadioBusyGpio(radio);

    const TickType_t timeout =
        pdMS_TO_TICKS(SX1262_SPI_TIMEOUT_MS);

    const TickType_t start =
        xTaskGetTickCount();

    while (gpio_get_level(busy_gpio) != 0)
    {
        if ((xTaskGetTickCount() - start) >= timeout)
        {
            return ESP_ERR_TIMEOUT;
        }

        vTaskDelay(
            pdMS_TO_TICKS(1U));
    }

    return ESP_OK;
}

/* -------------------------------------------------------------------------- */
/* SX1262 reset                                                               */
/* -------------------------------------------------------------------------- */

static esp_err_t gwRadioHardwareReset(
    GwRadioId_t radio)
{
    if (!gwRadioIdValid(radio))
    {
        return ESP_ERR_INVALID_ARG;
    }

    const gpio_num_t reset_gpio =
        gwRadioResetGpio(radio);

    gpio_set_level(
        reset_gpio,
        0);

    vTaskDelay(
        pdMS_TO_TICKS(
            SX1262_RESET_LOW_MS));

    gpio_set_level(
        reset_gpio,
        1);

    vTaskDelay(
        pdMS_TO_TICKS(
            SX1262_RESET_HIGH_WAIT_MS));

    return ESP_OK;
}

/* -------------------------------------------------------------------------- */
/* SX1262 command write                                                       */
/* -------------------------------------------------------------------------- */

static esp_err_t gwRadioWriteCommand(
    GwRadioId_t radio,
    uint8_t command,
    const uint8_t *data,
    size_t length)
{
    if (!gwRadioIdValid(radio) ||
        s_radio_handle[radio] == NULL ||
        (data == NULL && length != 0U))
    {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err =
        gwRadioWaitWhileBusy(radio);

    if (err != ESP_OK)
    {
        return err;
    }

    if ((length + 1U) > 256U)
    {
        return ESP_ERR_INVALID_SIZE;
    }

    uint8_t tx_data[256];

    tx_data[0] = command;

    if (length != 0U)
    {
        memcpy(
            &tx_data[1],
            data,
            length);
    }

    spi_transaction_t transaction = {
        .length = (length + 1U) * 8U,
        .tx_buffer = tx_data,
        .rx_buffer = NULL
    };

    err = spi_device_transmit(
        s_radio_handle[radio],
        &transaction);

    if (err != ESP_OK)
    {
        return err;
    }

    return ESP_OK;
}

/* -------------------------------------------------------------------------- */
/* SX1262 command read                                                        */
/* -------------------------------------------------------------------------- */

static esp_err_t gwRadioReadCommand(
    GwRadioId_t radio,
    uint8_t command,
    uint8_t *data,
    size_t length)
{
    if (!gwRadioIdValid(radio) ||
        s_radio_handle[radio] == NULL ||
        data == NULL ||
        length == 0U)
    {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err =
        gwRadioWaitWhileBusy(radio);

    if (err != ESP_OK)
    {
        return err;
    }

    if ((length + 1U) > 256U)
    {
        return ESP_ERR_INVALID_SIZE;
    }

    uint8_t tx_data[256];
    uint8_t rx_data[256];

    memset(
        tx_data,
        0,
        sizeof(tx_data));

    memset(
        rx_data,
        0,
        sizeof(rx_data));

    tx_data[0] = command;

    spi_transaction_t transaction = {
        .length = (length + 1U) * 8U,
        .tx_buffer = tx_data,
        .rx_buffer = rx_data
    };

    err = spi_device_transmit(
        s_radio_handle[radio],
        &transaction);

    if (err != ESP_OK)
    {
        return err;
    }

    memcpy(
        data,
        &rx_data[1],
        length);

    return ESP_OK;
}

/* -------------------------------------------------------------------------- */
/* SX1262 status                                                               */
/* -------------------------------------------------------------------------- */

static esp_err_t gwRadioGetStatus(
    GwRadioId_t radio,
    uint8_t *status)
{
    if (!gwRadioIdValid(radio) ||
        status == NULL ||
        s_radio_handle[radio] == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t rx_data = 0U;

    esp_err_t err =
        gwRadioReadCommand(
            radio,
            SX1262_CMD_GET_STATUS,
            &rx_data,
            1U);

    if (err != ESP_OK)
    {
        return err;
    }

    *status = rx_data;

    return ESP_OK;
}

/* -------------------------------------------------------------------------- */
/* SX1262 basic configuration                                                 */
/* -------------------------------------------------------------------------- */

static esp_err_t gwRadioSetStandby(
    GwRadioId_t radio)
{
    const uint8_t standby =
        SX1262_STANDBY_RC;

    return gwRadioWriteCommand(
        radio,
        SX1262_CMD_SET_STANDBY,
        &standby,
        1U);
}

static esp_err_t gwRadioSetPacketTypeLoRa(
    GwRadioId_t radio)
{
    const uint8_t packet_type =
        SX1262_PACKET_TYPE_LORA;

    return gwRadioWriteCommand(
        radio,
        SX1262_CMD_SET_PACKET_TYPE,
        &packet_type,
        1U);
}

static esp_err_t gwRadioSetRfFrequency(
    GwRadioId_t radio,
    uint32_t frequency_hz)
{
    /*
     * SX1262 RF frequency word:
     *
     * RF_FREQ = frequency_hz * 2^25 / 32 MHz
     */
    const uint64_t numerator =
        ((uint64_t)frequency_hz << 25U);

    const uint32_t rf_frequency =
        (uint32_t)(
            numerator / 32000000ULL);

    uint8_t data[4] = {
        (uint8_t)(rf_frequency >> 24U),
        (uint8_t)(rf_frequency >> 16U),
        (uint8_t)(rf_frequency >> 8U),
        (uint8_t)(rf_frequency)
    };

    return gwRadioWriteCommand(
        radio,
        SX1262_CMD_SET_RF_FREQUENCY,
        data,
        sizeof(data));
}

static esp_err_t gwRadioSetModulationParams(
    GwRadioId_t radio,
    const GwRadioConfig_t *config)
{
    /*
     * LoRa modulation parameter encoding:
     *
     * SF:
     *   SF5  = 0x05
     *   ...
     *   SF12 = 0x0C
     *
     * BW:
     *   0 = 125 kHz
     *   1 = 250 kHz
     *   2 = 500 kHz
     *
     * CR:
     *   1 = 4/5
     *   2 = 4/6
     *   3 = 4/7
     *   4 = 4/8
     */
    uint8_t bandwidth;

    switch (config->bandwidth)
    {
        case 125U:
            bandwidth = 0x04U;
            break;

        case 250U:
            bandwidth = 0x05U;
            break;

        case 500U:
            bandwidth = 0x06U;
            break;

        default:
            return ESP_ERR_INVALID_ARG;
    }

    uint8_t data[4] = {
        config->spreading_factor,
        bandwidth,
        config->coding_rate,
        0x00U
    };

    return gwRadioWriteCommand(
        radio,
        SX1262_CMD_SET_MODULATION_PARAMS,
        data,
        sizeof(data));
}

static esp_err_t gwRadioSetPacketParams(
    GwRadioId_t radio)
{
    /*
     * Fixed packet configuration for the current
     * transport layer:
     *
     * Preamble          = 8 symbols
     * Header            = explicit
     * Payload length    = variable
     * CRC               = enabled
     * IQ                = standard
     */
    uint8_t data[6] = {
        0x00U,
        0x08U,
        0x00U,
        0x00U,
        0x01U,
        0x00U
    };

    return gwRadioWriteCommand(
        radio,
        SX1262_CMD_SET_PACKET_PARAMS,
        data,
        sizeof(data));
}

static esp_err_t gwRadioSetTxParams(
    GwRadioId_t radio,
    int8_t power_dbm)
{
    if (power_dbm < -9 ||
        power_dbm > 22)
    {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t data[2] = {
        (uint8_t)power_dbm,
        0x04U
    };

    return gwRadioWriteCommand(
        radio,
        SX1262_CMD_SET_TX_PARAMS,
        data,
        sizeof(data));
}

static esp_err_t gwRadioSetBufferBaseAddress(
    GwRadioId_t radio)
{
    uint8_t data[2] = {
        SX1262_BUFFER_BASE_TX,
        SX1262_BUFFER_BASE_RX
    };

    return gwRadioWriteCommand(
        radio,
        SX1262_CMD_SET_BUFFER_BASE_ADDR,
        data,
        sizeof(data));
}

/* -------------------------------------------------------------------------- */
/* SX1262 initialization                                                       */
/* -------------------------------------------------------------------------- */

static esp_err_t gwRadioConfigureDevice(
    GwRadioId_t radio,
    const GwRadioConfig_t *config)
{
    esp_err_t err;

    err = gwRadioSetStandby(radio);

    if (err != ESP_OK)
    {
        return err;
    }

    err = gwRadioSetPacketTypeLoRa(radio);

    if (err != ESP_OK)
    {
        return err;
    }

    err = gwRadioSetRfFrequency(
        radio,
        config->frequency_hz);

    if (err != ESP_OK)
    {
        return err;
    }

    err = gwRadioSetModulationParams(
        radio,
        config);

    if (err != ESP_OK)
    {
        return err;
    }

    err = gwRadioSetPacketParams(radio);

    if (err != ESP_OK)
    {
        return err;
    }

    err = gwRadioSetTxParams(
        radio,
        config->tx_power_dbm);

    if (err != ESP_OK)
    {
        return err;
    }

    err = gwRadioSetBufferBaseAddress(radio);

    if (err != ESP_OK)
    {
        return err;
    }

    return ESP_OK;
}

/* -------------------------------------------------------------------------- */
/* Public lifecycle API                                                       */
/* -------------------------------------------------------------------------- */

GwResult_t gwRadioInit(
    const GwRadioConfig_t *config)
{
    if (config == NULL)
    {
        return GW_RESULT_INVALID_ARG;
    }

    if (config->frequency_hz < 850000000UL ||
        config->frequency_hz > 930000000UL)
    {
        return GW_RESULT_INVALID_ARG;
    }

    if (config->spreading_factor < 5U ||
        config->spreading_factor > 12U)
    {
        return GW_RESULT_INVALID_ARG;
    }

    if (config->coding_rate < 1U ||
        config->coding_rate > 4U)
    {
        return GW_RESULT_INVALID_ARG;
    }

    if (config->bandwidth != 125U &&
        config->bandwidth != 250U &&
        config->bandwidth != 500U)
    {
        return GW_RESULT_INVALID_ARG;
    }

    if (config->tx_power_dbm < -9 ||
        config->tx_power_dbm > 22)
    {
        return GW_RESULT_INVALID_ARG;
    }

    if (gwRadioAllInitialized())
    {
        return GW_RESULT_ALREADY_INITIALIZED;
    }

    esp_err_t err =
        gwRadioConfigureGpios();

    if (err != ESP_OK)
    {
        return GW_RESULT_ERROR;
    }

    err = gwRadioConfigureSpi();

    if (err != ESP_OK)
    {
        gwRadioCleanupSpi();
        return GW_RESULT_ERROR;
    }

    memcpy(
        &s_radio_config,
        config,
        sizeof(s_radio_config));

    for (size_t i = 0U;
         i < GW_RADIO_COUNT;
         ++i)
    {
        const GwRadioId_t radio =
            (GwRadioId_t)i;

        err = gwRadioHardwareReset(radio);

        if (err != ESP_OK)
        {
            gwRadioCleanupSpi();

            memset(
                s_radio_initialized,
                0,
                sizeof(s_radio_initialized));

            return GW_RESULT_ERROR;
        }

        uint8_t status = 0U;

        err = gwRadioGetStatus(
            radio,
            &status);

        if (err != ESP_OK)
        {
            gwRadioCleanupSpi();

            memset(
                s_radio_initialized,
                0,
                sizeof(s_radio_initialized));

            return GW_RESULT_ERROR;
        }

        (void)status;

        err = gwRadioConfigureDevice(
            radio,
            config);

        if (err != ESP_OK)
        {
            gwRadioCleanupSpi();

            memset(
                s_radio_initialized,
                0,
                sizeof(s_radio_initialized));

            return GW_RESULT_ERROR;
        }

        s_radio_initialized[radio] = true;
    }

    return GW_RESULT_OK;
}

GwResult_t gwRadioDeinit(void)
{
    bool any_initialized = false;

    for (size_t i = 0U;
         i < GW_RADIO_COUNT;
         ++i)
    {
        if (s_radio_initialized[i])
        {
            any_initialized = true;
            break;
        }
    }

    if (!any_initialized)
    {
        return GW_RESULT_NOT_INITIALIZED;
    }

    for (size_t i = 0U;
         i < GW_RADIO_COUNT;
         ++i)
    {
        s_radio_initialized[i] = false;
    }

    memset(
        &s_radio_config,
        0,
        sizeof(s_radio_config));

    gwRadioCleanupSpi();

    return GW_RESULT_OK;
}

bool gwRadioIsInitialized(
    GwRadioId_t radio)
{
    if (!gwRadioIdValid(radio))
    {
        return false;
    }

    return s_radio_initialized[radio];
}

bool gwRadioAllInitialized(void)
{
    for (size_t i = 0U;
         i < GW_RADIO_COUNT;
         ++i)
    {
        if (!s_radio_initialized[i])
        {
            return false;
        }
    }

    return true;
}

GwResult_t gwRadioGetConfig(
    GwRadioConfig_t *config)
{
    if (config == NULL)
    {
        return GW_RESULT_INVALID_ARG;
    }

    if (!gwRadioAllInitialized())
    {
        return GW_RESULT_NOT_INITIALIZED;
    }

    memcpy(
        config,
        &s_radio_config,
        sizeof(*config));

    return GW_RESULT_OK;
}

/* -------------------------------------------------------------------------- */
/* Hardware diagnostics                                                       */
/* -------------------------------------------------------------------------- */

GwResult_t gwRadioReset(
    GwRadioId_t radio)
{
    if (!gwRadioIdValid(radio))
    {
        return GW_RESULT_INVALID_ARG;
    }

    if (!s_radio_initialized[radio])
    {
        return GW_RESULT_NOT_INITIALIZED;
    }

    if (gwRadioHardwareReset(radio) != ESP_OK)
    {
        return GW_RESULT_ERROR;
    }

    return GW_RESULT_OK;
}

GwResult_t gwRadioGetBusy(
    GwRadioId_t radio,
    bool *busy)
{
    if (!gwRadioIdValid(radio) ||
        busy == NULL)
    {
        return GW_RESULT_INVALID_ARG;
    }

    if (!s_radio_initialized[radio])
    {
        return GW_RESULT_NOT_INITIALIZED;
    }

    *busy =
        (gpio_get_level(
            gwRadioBusyGpio(radio)) != 0);

    return GW_RESULT_OK;
}

GwResult_t gwRadioGetDio1(
    GwRadioId_t radio,
    bool *dio1)
{
    if (!gwRadioIdValid(radio) ||
        dio1 == NULL)
    {
        return GW_RESULT_INVALID_ARG;
    }

    if (!s_radio_initialized[radio])
    {
        return GW_RESULT_NOT_INITIALIZED;
    }

    *dio1 =
        (gpio_get_level(
            gwRadioDio1Gpio(radio)) != 0);

    return GW_RESULT_OK;
}

GwResult_t gwRadioSetRxTx(
    GwRadioId_t radio,
    bool rx_enable,
    bool tx_enable)
{
    if (!gwRadioIdValid(radio))
    {
        return GW_RESULT_INVALID_ARG;
    }

    if (rx_enable && tx_enable)
    {
        return GW_RESULT_INVALID_ARG;
    }

    if (!s_radio_initialized[radio])
    {
        return GW_RESULT_NOT_INITIALIZED;
    }

    const gpio_num_t rxen_gpio =
        gwRadioRxenGpio(radio);

    const gpio_num_t txen_gpio =
        gwRadioTxenGpio(radio);

    /*
     * Apply safe state first.
     */
    gpio_set_level(
        rxen_gpio,
        0);

    gpio_set_level(
        txen_gpio,
        0);

    if (rx_enable)
    {
        gpio_set_level(
            rxen_gpio,
            1);
    }
    else if (tx_enable)
    {
        gpio_set_level(
            txen_gpio,
            1);
    }

    return GW_RESULT_OK;
}

/* -------------------------------------------------------------------------- */
/* SX1262 TX                                                                  */
/* -------------------------------------------------------------------------- */

GwResult_t gwRadioSend(
    GwRadioId_t radio,
    const uint8_t *data,
    size_t length)
{
    if (!gwRadioIdValid(radio) ||
        data == NULL ||
        length == 0U)
    {
        return GW_RESULT_INVALID_ARG;
    }

    if (!s_radio_initialized[radio])
    {
        return GW_RESULT_NOT_INITIALIZED;
    }

    if (length > 255U)
    {
        return GW_RESULT_INVALID_ARG;
    }

    esp_err_t err;

    err = gwRadioSetRxTx(
        radio,
        false,
        true) == GW_RESULT_OK
        ? ESP_OK
        : ESP_FAIL;

    if (err != ESP_OK)
    {
        return GW_RESULT_ERROR;
    }

    /*
     * Write payload into the SX1262 buffer.
     */
    uint8_t tx_buffer[256];

    tx_buffer[0] = SX1262_BUFFER_BASE_TX;

    memcpy(
        &tx_buffer[1],
        data,
        length);

    err = gwRadioWriteCommand(
        radio,
        SX1262_CMD_WRITE_BUFFER,
        tx_buffer,
        length + 1U);

    if (err != ESP_OK)
    {
        (void)gwRadioSetRxTx(
            radio,
            false,
            false);

        return GW_RESULT_ERROR;
    }

    /*
     * Payload length must be updated in packet parameters
     * before transmission.
     */
    uint8_t packet_params[6] = {
        0x00U,
        0x08U,
        0x00U,
        (uint8_t)length,
        0x01U,
        0x00U
    };

    err = gwRadioWriteCommand(
        radio,
        SX1262_CMD_SET_PACKET_PARAMS,
        packet_params,
        sizeof(packet_params));

    if (err != ESP_OK)
    {
        (void)gwRadioSetRxTx(
            radio,
            false,
            false);

        return GW_RESULT_ERROR;
    }

    /*
     * SetTx timeout = 0xFFFFFF.
     */
    uint8_t tx_timeout[3] = {
        (uint8_t)(
            SX1262_TX_TIMEOUT_INFINITE >> 16U),
        (uint8_t)(
            SX1262_TX_TIMEOUT_INFINITE >> 8U),
        (uint8_t)(
            SX1262_TX_TIMEOUT_INFINITE)
    };

    err = gwRadioWriteCommand(
        radio,
        SX1262_CMD_SET_TX,
        tx_timeout,
        sizeof(tx_timeout));

    if (err != ESP_OK)
    {
        (void)gwRadioSetRxTx(
            radio,
            false,
            false);

        return GW_RESULT_ERROR;
    }

    return GW_RESULT_OK;
}

/* -------------------------------------------------------------------------- */
/* SX1262 RX                                                                  */
/* -------------------------------------------------------------------------- */

GwResult_t gwRadioReceive(
    GwRadioId_t radio,
    uint8_t *buffer,
    size_t buffer_size,
    size_t *received_length)
{
    if (!gwRadioIdValid(radio) ||
        buffer == NULL ||
        buffer_size == 0U ||
        received_length == NULL)
    {
        return GW_RESULT_INVALID_ARG;
    }

    if (!s_radio_initialized[radio])
    {
        return GW_RESULT_NOT_INITIALIZED;
    }

    if (buffer_size > 255U)
    {
        return GW_RESULT_INVALID_ARG;
    }

    *received_length = 0U;

    /*
     * RX mode.
     *
     * Timeout = 0 means single RX according to SX1262
     * command semantics.
     */
    uint8_t rx_timeout[3] = {
        (uint8_t)(
            SX1262_RX_TIMEOUT_SINGLE >> 16U),
        (uint8_t)(
            SX1262_RX_TIMEOUT_SINGLE >> 8U),
        (uint8_t)(
            SX1262_RX_TIMEOUT_SINGLE)
    };

    if (gwRadioSetRxTx(
            radio,
            true,
            false) != GW_RESULT_OK)
    {
        return GW_RESULT_ERROR;
    }

    esp_err_t err = gwRadioWriteCommand(
        radio,
        SX1262_CMD_SET_RX,
        rx_timeout,
        sizeof(rx_timeout));

    if (err != ESP_OK)
    {
        (void)gwRadioSetRxTx(
            radio,
            false,
            false);

        return GW_RESULT_ERROR;
    }

    /*
     * Check IRQ status.
     */
    uint8_t irq_data[2] = {
        0x00U,
        0x00U
    };

    err = gwRadioReadCommand(
        radio,
        SX1262_CMD_GET_IRQ_STATUS,
        irq_data,
        sizeof(irq_data));

    if (err != ESP_OK)
    {
        return GW_RESULT_ERROR;
    }

    const uint16_t irq_status =
        ((uint16_t)irq_data[0] << 8U) |
        irq_data[1];

    /*
     * RX Done = bit 6.
     */
    if ((irq_status & 0x0040U) == 0U)
    {
        return GW_RESULT_NOT_READY;
    }

    /*
     * GetRxBufferStatus:
     *
     * Byte 0 = payload length
     * Byte 1 = RX start buffer pointer
     */
    uint8_t rx_status[2] = {
        0x00U,
        0x00U
    };

    err = gwRadioReadCommand(
        radio,
        SX1262_CMD_GET_RX_BUFFER_STATUS,
        rx_status,
        sizeof(rx_status));

    if (err != ESP_OK)
    {
        return GW_RESULT_ERROR;
    }

    size_t payload_length =
        rx_status[0];

    if (payload_length > buffer_size)
    {
        payload_length = buffer_size;
    }

    /*
     * ReadBuffer takes an offset followed by dummy.
     */
    uint8_t read_command[1] = {
        rx_status[1]
    };

    if (payload_length == 0U)
    {
        return GW_RESULT_OK;
    }

    /*
     * ReadBuffer requires the offset byte followed by
     * the returned payload. Use a direct SPI transaction
     * here because gwRadioReadCommand inserts its own
     * dummy byte.
     */
    uint8_t tx_data[256];
    uint8_t rx_data[256];

    memset(
        tx_data,
        0,
        sizeof(tx_data));

    memset(
        rx_data,
        0,
        sizeof(rx_data));

    tx_data[0] = SX1262_CMD_READ_BUFFER;
    tx_data[1] = read_command[0];

    err = gwRadioWaitWhileBusy(radio);

    if (err != ESP_OK)
    {
        return GW_RESULT_ERROR;
    }

    spi_transaction_t transaction = {
        .length = (payload_length + 2U) * 8U,
        .tx_buffer = tx_data,
        .rx_buffer = rx_data
    };

    err = spi_device_transmit(
        s_radio_handle[radio],
        &transaction);

    if (err != ESP_OK)
    {
        return GW_RESULT_ERROR;
    }

    memcpy(
        buffer,
        &rx_data[2],
        payload_length);

    *received_length =
        payload_length;

    /*
     * Clear RX Done IRQ.
     */
    uint8_t clear_irq[2] = {
        0x00U,
        0x40U
    };

    (void)gwRadioWriteCommand(
        radio,
        SX1262_CMD_CLEAR_IRQ_STATUS,
        clear_irq,
        sizeof(clear_irq));

    return GW_RESULT_OK;
}