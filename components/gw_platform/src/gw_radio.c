#include "gw_radio.h"
#include "gw_radio_hw.h"

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_err.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <string.h>

#define SX1262_CMD_GET_STATUS     (0xC0U)

#define SX1262_RESET_LOW_MS       (2U)
#define SX1262_RESET_HIGH_WAIT_MS (10U)

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

/* -------------------------------------------------------------------------- */
/* GPIO                                                                        */
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

    esp_err_t err = gpio_config(&output_config);

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

    err = gpio_config(&input_config);

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
    gpio_set_level(GW_RADIO0_RESET_GPIO, 1);
    gpio_set_level(GW_RADIO1_RESET_GPIO, 1);

    gpio_set_level(GW_RADIO0_RXEN_GPIO, 0);
    gpio_set_level(GW_RADIO0_TXEN_GPIO, 0);

    gpio_set_level(GW_RADIO1_RXEN_GPIO, 0);
    gpio_set_level(GW_RADIO1_TXEN_GPIO, 0);

    return ESP_OK;
}

/* -------------------------------------------------------------------------- */
/* SPI                                                                         */
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
/* SX1262 reset                                                                */
/* -------------------------------------------------------------------------- */

static esp_err_t gwRadioHardwareReset(
    GwRadioId_t radio)
{
    gpio_num_t reset_gpio;

    if (radio == GW_RADIO_0)
    {
        reset_gpio = GW_RADIO0_RESET_GPIO;
    }
    else if (radio == GW_RADIO_1)
    {
        reset_gpio = GW_RADIO1_RESET_GPIO;
    }
    else
    {
        return ESP_ERR_INVALID_ARG;
    }

    gpio_set_level(reset_gpio, 0);

    vTaskDelay(
        pdMS_TO_TICKS(SX1262_RESET_LOW_MS));

    gpio_set_level(reset_gpio, 1);

    vTaskDelay(
        pdMS_TO_TICKS(SX1262_RESET_HIGH_WAIT_MS));

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

    /*
     * SX1262 GetStatus:
     *
     * Byte 0 = command
     * Byte 1 = dummy byte used to clock out status.
     */
    uint8_t tx_data[2] = {
        SX1262_CMD_GET_STATUS,
        0x00U
    };

    uint8_t rx_data[2] = {
        0x00U,
        0x00U
    };

    spi_transaction_t transaction = {
        .length = sizeof(tx_data) * 8U,
        .tx_buffer = tx_data,
        .rx_buffer = rx_data
    };

    esp_err_t err = spi_device_transmit(
        s_radio_handle[radio],
        &transaction);

    if (err != ESP_OK)
    {
        return err;
    }

    *status = rx_data[1];

    return ESP_OK;
}

/* -------------------------------------------------------------------------- */
/* Public API                                                                  */
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

    if (gwRadioAllInitialized())
    {
        return GW_RESULT_ALREADY_INITIALIZED;
    }

    esp_err_t err = gwRadioConfigureGpios();

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

    /*
     * M2:
     *
     * Reset and communicate with both SX1262 devices.
     */
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

        /*
         * M2 currently proves that the SPI transaction
         * completed successfully.
         *
         * Detailed SX1262 status decoding is intentionally
         * separate from this layer revision.
         */
        (void)status;

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

    /*
     * SX1262 transmit protocol is not implemented yet.
     */
    return GW_RESULT_NOT_READY;
}

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

    *received_length = 0U;

    /*
     * SX1262 receive protocol is not implemented yet.
     */
    return GW_RESULT_NOT_READY;
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