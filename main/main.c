#include <stdio.h>

#include "esp_log.h"

#include "gw_lifecycle.h"
#include "gw_platform.h"
#include "gw_communication.h"
#include "gw_runtime.h"
#include "gw_sim_test.h"
#include "gw_transport.h"

static const char *TAG = "GW_MAIN";

void app_main(void)
{
    GwResult_t result;

    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "ORB DRIVE LORA GATEWAY");
    ESP_LOGI(TAG, "ESP32-S3 / SIM TEST MODE");
    ESP_LOGI(TAG, "========================================");

    /* --------------------------------------------------------- */
    /* Platform initialization                                   */
    /* --------------------------------------------------------- */

    result = gwPlatformInit();

    if (result != GW_RESULT_OK)
    {
        ESP_LOGE(
            TAG,
            "Platform init failed: %d",
            result);

        return;
    }

    ESP_LOGI(
        TAG,
        "Platform init: OK");

    /* --------------------------------------------------------- */
    /* Gateway lifecycle initialization                          */
    /* --------------------------------------------------------- */

    result = gwLifecycleInit();

    if (result != GW_RESULT_OK)
    {
        ESP_LOGE(
            TAG,
            "Lifecycle init failed: %d",
            result);

        return;
    }

    ESP_LOGI(
        TAG,
        "Lifecycle init: OK");

    result = gwLifecycleSetStage(
        GW_LIFECYCLE_DRIVERS_INIT);

    if (result != GW_RESULT_OK)
    {
        ESP_LOGE(
            TAG,
            "Drivers stage failed: %d",
            result);

        return;
    }

    /* --------------------------------------------------------- */
    /* SIM transport                                             */
    /*                                                           */
    /* No SX1262 hardware is required.                           */
    /* --------------------------------------------------------- */

    GwTransportConfig_t transport_config = {
        .type = GW_TRANSPORT_SIM,
        .radio = GW_TRANSPORT_RADIO_AUTO
    };

    ESP_LOGI(
        TAG,
        "Initializing SIM transport...");

    result = gwCommunicationInit(
        &transport_config);

    if (result != GW_RESULT_OK)
    {
        ESP_LOGE(
            TAG,
            "SIM communication init FAILED: %d",
            result);

        return;
    }

    ESP_LOGI(
        TAG,
        "SIM communication: initialized");

    /* --------------------------------------------------------- */
    /* Gateway runtime initialization                            */
    /*                                                           */
    /* Runtime owns:                                             */
    /* - Node Manager                                             */
    /* - Command Service                                          */
    /* - Event Service                                            */
    /* --------------------------------------------------------- */

    result = gwRuntimeInit();

    if (result != GW_RESULT_OK)
    {
        ESP_LOGE(
            TAG,
            "Gateway runtime init FAILED: %d",
            result);

        (void)gwCommunicationDeinit();

        return;
    }

    if (!gwRuntimeIsReady())
    {
        ESP_LOGE(
            TAG,
            "Gateway runtime is not ready");

        (void)gwRuntimeDeinit();
        (void)gwCommunicationDeinit();

        return;
    }

    ESP_LOGI(
        TAG,
        "Gateway runtime: initialized");

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

        return;
    }

    ESP_LOGI(
        TAG,
        "Gateway application ready");

    /* --------------------------------------------------------- */
    /* Run SIM test                                              */
    /* --------------------------------------------------------- */

    ESP_LOGI(
        TAG,
        "Starting Gateway SIM test...");

    result = gwSimTestRun();

    if (result != GW_RESULT_OK)
    {
        ESP_LOGE(
            TAG,
            "========================================");

        ESP_LOGE(
            TAG,
            "Gateway SIM Test: FAIL (%d)",
            result);

        ESP_LOGE(
            TAG,
            "========================================");

        (void)gwRuntimeDeinit();
        (void)gwCommunicationDeinit();

        return;
    }

    ESP_LOGI(
        TAG,
        "========================================");

    ESP_LOGI(
        TAG,
        "Gateway SIM Test: PASS");

    ESP_LOGI(
        TAG,
        "========================================");

    /* --------------------------------------------------------- */
    /* Cleanup                                                    */
    /* --------------------------------------------------------- */

    result = gwRuntimeDeinit();

    if (result != GW_RESULT_OK)
    {
        ESP_LOGE(
            TAG,
            "Runtime deinit failed: %d",
            result);
    }

    result = gwCommunicationDeinit();

    if (result != GW_RESULT_OK)
    {
        ESP_LOGE(
            TAG,
            "Communication deinit failed: %d",
            result);
    }

    ESP_LOGI(
        TAG,
        "Gateway SIM test complete");
}