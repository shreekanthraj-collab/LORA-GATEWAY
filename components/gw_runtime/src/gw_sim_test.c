#include "gw_sim_test.h"

#include "esp_log.h"

#include "gw_command_service.h"
#include "gw_communication.h"
#include "gw_event_service.h"
#include "gw_node_manager.h"
#include "gw_protocol.h"
#include "gw_transport.h"

#include <string.h>

static const char *TAG = "GW_SIM_TEST";

#define GW_SIM_TEST_NODE_ID       0x01u
#define GW_SIM_TEST_GATEWAY_ID    0x01u

/* -------------------------------------------------------------------------- */
/* Test helper                                                                */
/* -------------------------------------------------------------------------- */

static GwResult_t gwSimTestCheck(
    const char *name,
    GwResult_t result)
{
    if (result != GW_RESULT_OK)
    {
        ESP_LOGE(
            TAG,
            "[FAIL] %s: %d",
            name,
            result);

        return result;
    }

    ESP_LOGI(
        TAG,
        "[PASS] %s",
        name);

    return GW_RESULT_OK;
}

/* -------------------------------------------------------------------------- */
/* SIM transport loopback                                                     */
/* -------------------------------------------------------------------------- */

static GwResult_t gwSimTestTransportLoopback(void)
{
    GwTransportConfig_t config = {
        .type = GW_TRANSPORT_SIM,
        .radio = GW_TRANSPORT_RADIO_AUTO
    };

    uint8_t tx_packet[] = {
        GW_SIM_TEST_NODE_ID,
        GW_PKT_CMD,
        GW_CMD_OPEN,
        0x00u,
        0x01u,
        GW_SIM_TEST_GATEWAY_ID,
        0x00u
    };

    uint8_t rx_packet[GW_COMM_MAX_PACKET_SIZE];

    size_t received_length = 0u;

    GwResult_t result;

    /* ---------------------------------------------------------------------- */
    /* Initialize SIM transport                                              */
    /* ---------------------------------------------------------------------- */

    result = gwTransportInit(&config);

    if (result != GW_RESULT_OK)
    {
        ESP_LOGE(
            TAG,
            "SIM transport initialization FAILED: %d",
            result);

        return result;
    }

    result = gwSimTestCheck(
        "SIM transport initialization",
        result);

    if (result != GW_RESULT_OK)
    {
        (void)gwTransportDeinit();
        return result;
    }

    /* ---------------------------------------------------------------------- */
    /* Build command packet                                                   */
    /* ---------------------------------------------------------------------- */

    tx_packet[sizeof(tx_packet) - 1u] =
        gwPacketCrc8(
            tx_packet,
            sizeof(tx_packet) - 1u);

    /* ---------------------------------------------------------------------- */
    /* SIM transmit                                                           */
    /* ---------------------------------------------------------------------- */

    result = gwTransportSend(
        tx_packet,
        sizeof(tx_packet));

    if (result != GW_RESULT_OK)
    {
        ESP_LOGE(
            TAG,
            "SIM transport send FAILED: %d",
            result);

        (void)gwTransportDeinit();

        return result;
    }

    result = gwSimTestCheck(
        "SIM transport send",
        result);

    if (result != GW_RESULT_OK)
    {
        (void)gwTransportDeinit();
        return result;
    }

    /* ---------------------------------------------------------------------- */
    /* SIM receive                                                            */
    /* ---------------------------------------------------------------------- */

    memset(
        rx_packet,
        0,
        sizeof(rx_packet));

    result = gwTransportReceive(
        rx_packet,
        sizeof(rx_packet),
        &received_length);

    if (result != GW_RESULT_OK)
    {
        ESP_LOGE(
            TAG,
            "SIM transport receive FAILED: %d",
            result);

        (void)gwTransportDeinit();

        return result;
    }

    /* ---------------------------------------------------------------------- */
    /* Verify loopback packet                                                 */
    /* ---------------------------------------------------------------------- */

    if (received_length != sizeof(tx_packet))
    {
        ESP_LOGE(
            TAG,
            "[FAIL] SIM loopback length: expected=%u received=%u",
            (unsigned)sizeof(tx_packet),
            (unsigned)received_length);

        (void)gwTransportDeinit();

        return GW_RESULT_ERROR;
    }

    if (memcmp(
            tx_packet,
            rx_packet,
            sizeof(tx_packet)) != 0)
    {
        ESP_LOGE(
            TAG,
            "[FAIL] SIM loopback packet data");

        (void)gwTransportDeinit();

        return GW_RESULT_ERROR;
    }

    if (!gwPacketCrcValid(
            rx_packet,
            received_length))
    {
        ESP_LOGE(
            TAG,
            "[FAIL] SIM loopback packet CRC");

        (void)gwTransportDeinit();

        return GW_RESULT_ERROR;
    }

    ESP_LOGI(
        TAG,
        "[PASS] SIM loopback packet and CRC");

    /* ---------------------------------------------------------------------- */
    /* Deinitialize SIM transport                                            */
    /* ---------------------------------------------------------------------- */

    result = gwTransportDeinit();

    if (result != GW_RESULT_OK)
    {
        ESP_LOGE(
            TAG,
            "SIM transport deinitialization FAILED: %d",
            result);

        return result;
    }

    ESP_LOGI(
        TAG,
        "[PASS] SIM transport deinitialization");

    return GW_RESULT_OK;
}

/* -------------------------------------------------------------------------- */
/* Command Service                                                            */
/* -------------------------------------------------------------------------- */

static GwResult_t gwSimTestCommandService(void)
{
    GwTransportConfig_t config = {
        .type = GW_TRANSPORT_SIM,
        .radio = GW_TRANSPORT_RADIO_AUTO
    };

    GwCommandRequest_t request = {
        .node = GW_SIM_TEST_NODE_ID,
        .command = GW_CMD_OPEN,
        .value = 0u
    };

    uint8_t rx_packet[GW_COMM_MAX_PACKET_SIZE];

    size_t received_length = 0u;

    GwLoRaCmd_t packet;

    GwResult_t result;

    /* ---------------------------------------------------------------------- */
    /* Initialize communication using SIM transport                           */
    /* ---------------------------------------------------------------------- */

    result = gwCommunicationInit(
        &config);

    if (result != GW_RESULT_OK)
    {
        ESP_LOGE(
            TAG,
            "Communication SIM initialization FAILED: %d",
            result);

        return result;
    }

    /* ---------------------------------------------------------------------- */
    /* Send command through Command Service                                   */
    /* ---------------------------------------------------------------------- */

    result = gwCommandServiceSend(
        &request);

    if (result != GW_RESULT_OK)
    {
        ESP_LOGE(
            TAG,
            "Command Service send FAILED: %d",
            result);

        (void)gwCommunicationDeinit();

        return result;
    }

    /* ---------------------------------------------------------------------- */
    /* Receive command from SIM loopback                                      */
    /* ---------------------------------------------------------------------- */

    result = gwCommunicationReceive(
        rx_packet,
        sizeof(rx_packet),
        &received_length);

    if (result != GW_RESULT_OK)
    {
        ESP_LOGE(
            TAG,
            "Command Service receive FAILED: %d",
            result);

        (void)gwCommunicationDeinit();

        return result;
    }

    /* ---------------------------------------------------------------------- */
    /* Validate command packet                                                */
    /* ---------------------------------------------------------------------- */

    if (received_length != sizeof(GwLoRaCmd_t))
    {
        ESP_LOGE(
            TAG,
            "[FAIL] Command packet length: expected=%u received=%u",
            (unsigned)sizeof(GwLoRaCmd_t),
            (unsigned)received_length);

        (void)gwCommunicationDeinit();

        return GW_RESULT_ERROR;
    }

    if (!gwPacketCrcValid(
            rx_packet,
            received_length))
    {
        ESP_LOGE(
            TAG,
            "[FAIL] Command packet CRC");

        (void)gwCommunicationDeinit();

        return GW_RESULT_ERROR;
    }

    memcpy(
        &packet,
        rx_packet,
        sizeof(packet));

    if (packet.node != GW_SIM_TEST_NODE_ID ||
        packet.type != GW_PKT_CMD ||
        packet.cmd != GW_CMD_OPEN)
    {
        ESP_LOGE(
            TAG,
            "[FAIL] Command packet fields");

        (void)gwCommunicationDeinit();

        return GW_RESULT_ERROR;
    }

    ESP_LOGI(
        TAG,
        "[PASS] Command packet fields");

    /* ---------------------------------------------------------------------- */
    /* Deinitialize communication                                             */
    /* ---------------------------------------------------------------------- */

    result = gwCommunicationDeinit();

    if (result != GW_RESULT_OK)
    {
        return result;
    }

    return GW_RESULT_OK;
}

/* -------------------------------------------------------------------------- */
/* ACK processing                                                             */
/* -------------------------------------------------------------------------- */

static GwResult_t gwSimTestAck(void)
{
    GwLoRaAck_t ack = {0};

    /* ---------------------------------------------------------------------- */
    /* Build valid ACK                                                        */
    /* ---------------------------------------------------------------------- */

    ack.node = GW_SIM_TEST_NODE_ID;
    ack.type = GW_PKT_ACK;
    ack.seq = 1u;
    ack.result = GW_ACK_OK;
    ack.gwid = GW_SIM_TEST_GATEWAY_ID;

    ack.crc8 = gwPacketCrc8(
        (const uint8_t *)&ack,
        sizeof(ack) - 1u);

    /* ---------------------------------------------------------------------- */
    /* Validate ACK length                                                    */
    /* ---------------------------------------------------------------------- */

    if (sizeof(ack) != 6u)
    {
        ESP_LOGE(
            TAG,
            "[FAIL] ACK packet length");

        return GW_RESULT_ERROR;
    }

    ESP_LOGI(
        TAG,
        "[PASS] ACK packet length");

    /* ---------------------------------------------------------------------- */
    /* Validate ACK CRC                                                       */
    /* ---------------------------------------------------------------------- */

    if (!gwPacketCrcValid(
            (const uint8_t *)&ack,
            sizeof(ack)))
    {
        ESP_LOGE(
            TAG,
            "[FAIL] ACK valid CRC");

        return GW_RESULT_ERROR;
    }

    ESP_LOGI(
        TAG,
        "[PASS] ACK valid CRC");

    /* ---------------------------------------------------------------------- */
    /* Validate ACK fields                                                    */
    /* ---------------------------------------------------------------------- */

    if (ack.node != GW_SIM_TEST_NODE_ID ||
        ack.type != GW_PKT_ACK ||
        ack.seq != 1u ||
        ack.result != GW_ACK_OK ||
        ack.gwid != GW_SIM_TEST_GATEWAY_ID)
    {
        ESP_LOGE(
            TAG,
            "[FAIL] ACK fields");

        return GW_RESULT_ERROR;
    }

    ESP_LOGI(
        TAG,
        "[PASS] ACK fields");

    /* ---------------------------------------------------------------------- */
    /* Test ACK_DENIED                                                        */
    /* ---------------------------------------------------------------------- */

    ack.result = GW_ACK_DENIED;

    ack.crc8 = gwPacketCrc8(
        (const uint8_t *)&ack,
        sizeof(ack) - 1u);

    if (!gwPacketCrcValid(
            (const uint8_t *)&ack,
            sizeof(ack)))
    {
        ESP_LOGE(
            TAG,
            "[FAIL] ACK_DENIED CRC");

        return GW_RESULT_ERROR;
    }

    ESP_LOGI(
        TAG,
        "[PASS] ACK_DENIED");

    /* ---------------------------------------------------------------------- */
    /* Test ACK_FAULT                                                         */
    /* ---------------------------------------------------------------------- */

    ack.result = GW_ACK_FAULT;

    ack.crc8 = gwPacketCrc8(
        (const uint8_t *)&ack,
        sizeof(ack) - 1u);

    if (!gwPacketCrcValid(
            (const uint8_t *)&ack,
            sizeof(ack)))
    {
        ESP_LOGE(
            TAG,
            "[FAIL] ACK_FAULT CRC");

        return GW_RESULT_ERROR;
    }

    ESP_LOGI(
        TAG,
        "[PASS] ACK_FAULT");

    /* ---------------------------------------------------------------------- */
    /* Test bad CRC rejection                                                 */
    /* ---------------------------------------------------------------------- */

    ack.result = GW_ACK_OK;

    ack.crc8 = gwPacketCrc8(
        (const uint8_t *)&ack,
        sizeof(ack) - 1u);

    ack.crc8 ^= 0x01u;

    if (gwPacketCrcValid(
            (const uint8_t *)&ack,
            sizeof(ack)))
    {
        ESP_LOGE(
            TAG,
            "[FAIL] ACK bad CRC was accepted");

        return GW_RESULT_ERROR;
    }

    ESP_LOGI(
        TAG,
        "[PASS] ACK bad CRC rejected");

    return GW_RESULT_OK;
}

/* -------------------------------------------------------------------------- */
/* STATUS and EVENT processing                                                */
/* -------------------------------------------------------------------------- */

static GwResult_t gwSimTestStatusEvent(void)
{
    GwLoRaStatus_t status = {0};
    GwLoRaEvent_t event = {0};

    GwEventMessage_t message;

    GwNodeInfo_t info;

    GwNodeTelemetry_t telemetry;

    GwResult_t result;

    /* ---------------------------------------------------------------------- */
    /* Build STATUS packet                                                    */
    /* ---------------------------------------------------------------------- */

    status.node = GW_SIM_TEST_NODE_ID;
    status.type = GW_PKT_STATUS;
    status.seq = 1u;
    status.motor_state = 1u;
    status.turns100 = 1250u;
    status.voltage100 = 2400u;
    status.current100 = 350u;
    status.rssi = -55;
    status.gwid = GW_SIM_TEST_GATEWAY_ID;

    status.crc8 = gwPacketCrc8(
        (const uint8_t *)&status,
        sizeof(status) - 1u);

    message.type = GW_EVENT_TYPE_STATUS;
    message.data = (const uint8_t *)&status;
    message.length = sizeof(status);

    /* ---------------------------------------------------------------------- */
    /* Process STATUS                                                         */
    /* ---------------------------------------------------------------------- */

    result = gwEventServiceProcess(
        &message);

    if (result != GW_RESULT_OK)
    {
        ESP_LOGE(
            TAG,
            "STATUS processing FAILED: %d",
            result);

        return result;
    }

    /* ---------------------------------------------------------------------- */
    /* Verify Node Manager state                                              */
    /* ---------------------------------------------------------------------- */

    result = gwNodeManagerGet(
        GW_SIM_TEST_NODE_ID,
        &info);

    if (result != GW_RESULT_OK)
    {
        return result;
    }

    if (info.state != GW_NODE_STATE_ONLINE)
    {
        ESP_LOGE(
            TAG,
            "[FAIL] STATUS did not set node ONLINE");

        return GW_RESULT_ERROR;
    }

    ESP_LOGI(
        TAG,
        "[PASS] STATUS node ONLINE");

    /* ---------------------------------------------------------------------- */
    /* Verify STATUS telemetry                                                */
    /* ---------------------------------------------------------------------- */

    result = gwNodeManagerGetTelemetry(
        GW_SIM_TEST_NODE_ID,
        &telemetry);

    if (result != GW_RESULT_OK)
    {
        return result;
    }

    if (!telemetry.valid ||
        telemetry.motor_state != status.motor_state ||
        telemetry.turns100 != status.turns100 ||
        telemetry.voltage100 != status.voltage100 ||
        telemetry.current100 != status.current100 ||
        telemetry.rssi != status.rssi)
    {
        ESP_LOGE(
            TAG,
            "[FAIL] STATUS telemetry");

        return GW_RESULT_ERROR;
    }

    ESP_LOGI(
        TAG,
        "[PASS] STATUS telemetry");

    /* ---------------------------------------------------------------------- */
    /* Build EVENT packet                                                     */
    /* ---------------------------------------------------------------------- */

    event.node = GW_SIM_TEST_NODE_ID;
    event.type = GW_PKT_EVENT;
    event.seq = 2u;
    event.event = GW_EVT_OPEN_DONE;
    event.voltage100 = 2410u;
    event.current100 = 360u;
    event.gwid = GW_SIM_TEST_GATEWAY_ID;

    event.crc8 = gwPacketCrc8(
        (const uint8_t *)&event,
        sizeof(event) - 1u);

    message.type = GW_EVENT_TYPE_EVENT;
    message.data = (const uint8_t *)&event;
    message.length = sizeof(event);

    /* ---------------------------------------------------------------------- */
    /* Process EVENT                                                          */
    /* ---------------------------------------------------------------------- */

    result = gwEventServiceProcess(
        &message);

    if (result != GW_RESULT_OK)
    {
        ESP_LOGE(
            TAG,
            "EVENT processing FAILED: %d",
            result);

        return result;
    }

    /* ---------------------------------------------------------------------- */
    /* Verify EVENT telemetry                                                 */
    /* ---------------------------------------------------------------------- */

    result = gwNodeManagerGetTelemetry(
        GW_SIM_TEST_NODE_ID,
        &telemetry);

    if (result != GW_RESULT_OK)
    {
        return result;
    }

    if (!telemetry.valid ||
        telemetry.voltage100 != event.voltage100 ||
        telemetry.current100 != event.current100)
    {
        ESP_LOGE(
            TAG,
            "[FAIL] EVENT telemetry");

        return GW_RESULT_ERROR;
    }

    ESP_LOGI(
        TAG,
        "[PASS] EVENT telemetry");

    return GW_RESULT_OK;
}

/* -------------------------------------------------------------------------- */
/* Fault STATUS                                                               */
/* -------------------------------------------------------------------------- */

static GwResult_t gwSimTestFaultStatus(void)
{
    GwLoRaStatus_t packet = {0};

    GwEventMessage_t message;

    GwNodeInfo_t info;

    GwResult_t result;

    /* ---------------------------------------------------------------------- */
    /* Build fault STATUS packet                                              */
    /* ---------------------------------------------------------------------- */

    packet.node = GW_SIM_TEST_NODE_ID;
    packet.type = GW_PKT_STATUS;
    packet.seq = 3u;
    packet.motor_state = 1u;
    packet.fault_flags = GW_FAULT_OC;
    packet.turns100 = 1250u;
    packet.voltage100 = 2400u;
    packet.current100 = 900u;
    packet.rssi = -60;
    packet.gwid = GW_SIM_TEST_GATEWAY_ID;

    packet.crc8 = gwPacketCrc8(
        (const uint8_t *)&packet,
        sizeof(packet) - 1u);

    message.type = GW_EVENT_TYPE_STATUS;
    message.data = (const uint8_t *)&packet;
    message.length = sizeof(packet);

    /* ---------------------------------------------------------------------- */
    /* Process fault STATUS                                                   */
    /* ---------------------------------------------------------------------- */

    result = gwEventServiceProcess(
        &message);

    if (result != GW_RESULT_OK)
    {
        ESP_LOGE(
            TAG,
            "Fault STATUS processing FAILED: %d",
            result);

        return result;
    }

    /* ---------------------------------------------------------------------- */
    /* Verify FAULT state                                                     */
    /* ---------------------------------------------------------------------- */

    result = gwNodeManagerGet(
        GW_SIM_TEST_NODE_ID,
        &info);

    if (result != GW_RESULT_OK)
    {
        return result;
    }

    if (info.state != GW_NODE_STATE_FAULT)
    {
        ESP_LOGE(
            TAG,
            "[FAIL] Fault STATUS did not set node FAULT");

        return GW_RESULT_ERROR;
    }

    ESP_LOGI(
        TAG,
        "[PASS] Fault STATUS -> node FAULT");

    return GW_RESULT_OK;
}

/* -------------------------------------------------------------------------- */
/* Full Gateway SIM test                                                     */
/* -------------------------------------------------------------------------- */

GwResult_t gwSimTestRun(void)
{
    GwResult_t result;

    ESP_LOGI(
        TAG,
        "========================================");

    ESP_LOGI(
        TAG,
        "Gateway SIM Test");

    ESP_LOGI(
        TAG,
        "========================================");

    /* ---------------------------------------------------------------------- */
    /* Transport                                                               */
    /* ---------------------------------------------------------------------- */

    result = gwSimTestTransportLoopback();

    if (result != GW_RESULT_OK)
    {
        return result;
    }

    /* ---------------------------------------------------------------------- */
    /* Command Service                                                         */
    /* ---------------------------------------------------------------------- */

    result = gwSimTestCommandService();

    if (result != GW_RESULT_OK)
    {
        return result;
    }

    /* ---------------------------------------------------------------------- */
    /* ACK                                                                      */
    /* ---------------------------------------------------------------------- */

    result = gwSimTestAck();

    if (result != GW_RESULT_OK)
    {
        return result;
    }

    /* ---------------------------------------------------------------------- */
    /* Runtime services must already be initialized                           */
    /* ---------------------------------------------------------------------- */

    if (!gwNodeManagerIsInitialized() ||
        !gwCommandServiceIsInitialized() ||
        !gwEventServiceIsInitialized())
    {
        ESP_LOGE(
            TAG,
            "[FAIL] Gateway runtime services are not initialized");

        return GW_RESULT_NOT_READY;
    }

    /* ---------------------------------------------------------------------- */
    /* Register test node                                                      */
    /* ---------------------------------------------------------------------- */

    result = gwNodeManagerRegister(
        GW_SIM_TEST_NODE_ID,
        GW_SIM_TEST_GATEWAY_ID);

    if (result != GW_RESULT_OK)
    {
        ESP_LOGE(
            TAG,
            "SIM test node registration FAILED: %d",
            result);

        return result;
    }

    ESP_LOGI(
        TAG,
        "[PASS] SIM test node registration");

    /* ---------------------------------------------------------------------- */
    /* STATUS + EVENT                                                          */
    /* ---------------------------------------------------------------------- */

    result = gwSimTestStatusEvent();

    if (result != GW_RESULT_OK)
    {
        return result;
    }

    /* ---------------------------------------------------------------------- */
    /* Fault handling                                                          */
    /* ---------------------------------------------------------------------- */

    result = gwSimTestFaultStatus();

    if (result != GW_RESULT_OK)
    {
        return result;
    }

    /* ---------------------------------------------------------------------- */
    /* Complete                                                                 */
    /* ---------------------------------------------------------------------- */

    ESP_LOGI(
        TAG,
        "========================================");

    ESP_LOGI(
        TAG,
        "Gateway SIM Test: PASS");

    ESP_LOGI(
        TAG,
        "========================================");

    return GW_RESULT_OK;
}