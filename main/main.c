#include <stdio.h>

#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "gw_lifecycle.h"
#include "gw_platform.h"
#include "gw_radio.h"
#include "gw_communication.h"
#include "gw_runtime.h"
#include "gw_transport.h"

static const char *TAG = "GW_MAIN";

void app_main(void)
{
    GwResult_t result;

    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "ORB DRIVE LORA GATEWAY");
    ESP_LOGI(TAG, "ESP32-S3 / DUAL SX1262 RADIO MODE");
    ESP_LOGI(TAG, "========================================");

    /* --------------------------------------------------------- */
    /* Platform initialization                                   */
    /* --------------------------------------------------------- */

    result = gwPlatformInit();

    if (result != GW_RESULT_OK)
    {
        ESP_LOGE(TAG, "Platform init failed: %d", result);
        return;
    }

    ESP_LOGI(TAG, "Platform init: OK");

    /* --------------------------------------------------------- */
    /* Gateway lifecycle initialization                          */
    /* --------------------------------------------------------- */

    result = gwLifecycleInit();

    if (result != GW_RESULT_OK)
    {
        ESP_LOGE(TAG, "Lifecycle init failed: %d", result);
        (void)gwPlatformDeinit();
        return;
    }

    ESP_LOGI(TAG, "Lifecycle init: OK");

    result = gwLifecycleSetStage(
        GW_LIFECYCLE_DRIVERS_INIT);

    if (result != GW_RESULT_OK)
    {
        ESP_LOGE(TAG, "Drivers stage failed: %d", result);
        (void)gwPlatformDeinit();
        return;
    }

    /* --------------------------------------------------------- */
    /* Dual SX1262 radio initialization                          */
    /* --------------------------------------------------------- */

    const GwRadioConfig_t radio_config = {
        .frequency_hz = 915000000U,
        .bandwidth = 125U,
        .spreading_factor = 7U,
        .coding_rate = 1U,
        .tx_power_dbm = 14
    };

    ESP_LOGI(TAG, "Initializing dual SX1262 radios...");

    result = gwRadioInit(&radio_config);

    if (result != GW_RESULT_OK)
    {
        ESP_LOGE(TAG, "Radio init FAILED: %d", result);
        (void)gwPlatformDeinit();
        return;
    }

    if (!gwRadioAllInitialized())
    {
        ESP_LOGE(TAG, "Not all SX1262 radios initialized");
        (void)gwRadioDeinit();
        (void)gwPlatformDeinit();
        return;
    }

    ESP_LOGI(TAG, "Dual SX1262 radios: initialized");

    /* --------------------------------------------------------- */
    /* Radio transport                                           */
    /* --------------------------------------------------------- */

    const GwTransportConfig_t transport_config = {
        .type = GW_TRANSPORT_RADIO,
        .radio = GW_TRANSPORT_RADIO_AUTO
    };

    ESP_LOGI(TAG, "Initializing RADIO transport...");

    result = gwCommunicationInit(
        &transport_config);

    if (result != GW_RESULT_OK)
    {
        ESP_LOGE(
            TAG,
            "Radio communication init FAILED: %d",
            result);

        (void)gwRadioDeinit();
        (void)gwPlatformDeinit();
        return;
    }

    ESP_LOGI(TAG, "RADIO communication: initialized");

    /* --------------------------------------------------------- */
    /* Gateway runtime initialization                            */
    /* --------------------------------------------------------- */

    result = gwRuntimeInit();

    if (result != GW_RESULT_OK)
    {
        ESP_LOGE(
            TAG,
            "Gateway runtime init FAILED: %d",
            result);

        (void)gwCommunicationDeinit();
        (void)gwRadioDeinit();
        (void)gwPlatformDeinit();
        return;
    }

    if (!gwRuntimeIsReady())
    {
        ESP_LOGE(TAG, "Gateway runtime is not ready");

        (void)gwRuntimeDeinit();
        (void)gwCommunicationDeinit();
        (void)gwRadioDeinit();
        (void)gwPlatformDeinit();
        return;
    }

    ESP_LOGI(TAG, "Gateway runtime: initialized");

    /* --------------------------------------------------------- */
    /* Application ready                                         */
    /* --------------------------------------------------------- */

    result = gwLifecycleSetStage(
        GW_LIFECYCLE_APPLICATION_READY);

    if (result != GW_RESULT_OK)
    {
        ESP_LOGE(
            TAG,
            "Application-ready stage failed: %d",
            result);

        (void)gwRuntimeDeinit();
        (void)gwCommunicationDeinit();
        (void)gwRadioDeinit();
        (void)gwPlatformDeinit();
        return;
    }

    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Gateway application ready");
    ESP_LOGI(TAG, "Dual SX1262 radio startup complete");
    ESP_LOGI(TAG, "========================================");

    /* --------------------------------------------------------- */
    /* Gateway main loop                                         */
    /* --------------------------------------------------------- */

    while (true)
    {
        vTaskDelay(
            pdMS_TO_TICKS(1000U));
    }
}
