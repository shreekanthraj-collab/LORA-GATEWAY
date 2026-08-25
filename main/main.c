#include <stdio.h>

#include "esp_log.h"

#include "gw_lifecycle.h"
#include "gw_platform.h"
#include "gw_radio.h"
#include "gw_radio_hw.h"
#include "gw_communication.h"
#include "gw_transport.h"

static const char *TAG = "GW_MAIN";

void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "ORB DRIVE LORA GATEWAY");
    ESP_LOGI(TAG, "ESP32-S3 / Dual SX1262");
    ESP_LOGI(TAG, "========================================");

    GwResult_t result;

    /* ---------------------------------------------------------
     * Platform initialization
     * --------------------------------------------------------- */
    result = gwPlatformInit();

    if (result != GW_RESULT_OK)
    {
        ESP_LOGE(TAG, "Platform init failed: %d", result);
        return;
    }

    ESP_LOGI(TAG, "Platform init: OK");

    /* ---------------------------------------------------------
     * Gateway lifecycle initialization
     * --------------------------------------------------------- */
    result = gwLifecycleInit();

    if (result != GW_RESULT_OK)
    {
        ESP_LOGE(TAG, "Lifecycle init failed: %d", result);
        return;
    }

    ESP_LOGI(TAG, "Lifecycle init: OK");

    result = gwLifecycleSetStage(
        GW_LIFECYCLE_DRIVERS_INIT);

    if (result != GW_RESULT_OK)
    {
        ESP_LOGE(TAG, "Drivers stage failed: %d", result);
        return;
    }

    /* ---------------------------------------------------------
     * Dual SX1262 initialization
     * --------------------------------------------------------- */
    GwRadioConfig_t radio_config = {
        .frequency_hz = GW_RADIO0_FREQUENCY_HZ,
        .bandwidth = 0U,
        .spreading_factor = 7U,
        .coding_rate = 1U,
        .tx_power_dbm = 17
    };

    ESP_LOGI(TAG, "Initializing dual SX1262 radios...");

    result = gwRadioInit(&radio_config);

    if (result != GW_RESULT_OK)
    {
        ESP_LOGE(TAG, "Dual radio init FAILED: %d", result);
        return;
    }

    ESP_LOGI(TAG, "Radio A: initialized");
    ESP_LOGI(TAG, "Radio B: initialized");

    result = gwLifecycleSetStage(
        GW_LIFECYCLE_RADIO_INIT);

    if (result != GW_RESULT_OK)
    {
        ESP_LOGE(TAG, "Radio lifecycle stage failed: %d", result);
        return;
    }

    /* ---------------------------------------------------------
     * Gateway communication initialization
     *
     * Radio hardware is already initialized above.
     * The communication layer therefore owns the transport
     * abstraction but does not own radio hardware lifetime.
     * --------------------------------------------------------- */
    GwTransportConfig_t transport_config = {
        .type = GW_TRANSPORT_RADIO,
        .radio = GW_TRANSPORT_RADIO_AUTO
    };

    result = gwCommunicationInit(
        &transport_config);

    if (result != GW_RESULT_OK)
    {
        ESP_LOGE(
            TAG,
            "Communication init FAILED: %d",
            result);
        return;
    }

    ESP_LOGI(TAG, "Communication layer: initialized");

    /* ---------------------------------------------------------
     * Application ready
     * --------------------------------------------------------- */
    result = gwLifecycleSetStage(
        GW_LIFECYCLE_APPLICATION_READY);

    if (result != GW_RESULT_OK)
    {
        ESP_LOGE(
            TAG,
            "Application-ready stage failed: %d",
            result);
        return;
    }

    ESP_LOGI(TAG, "Gateway application ready");

    /* ---------------------------------------------------------
     * Gateway RUN
     * --------------------------------------------------------- */
    result = gwLifecycleSetStage(
        GW_LIFECYCLE_RUN);

    if (result != GW_RESULT_OK)
    {
        ESP_LOGE(TAG, "RUN stage failed: %d", result);
        return;
    }

    ESP_LOGI(TAG, "Gateway RUN");

    ESP_LOGI(
        TAG,
        "LoRa A frequency: %lu Hz",
        (unsigned long)GW_RADIO0_FREQUENCY_HZ);

    ESP_LOGI(
        TAG,
        "LoRa B frequency: %lu Hz",
        (unsigned long)GW_RADIO1_FREQUENCY_HZ);
}
