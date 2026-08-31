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

    result = gwTransportInit(&config);

    if (result != GW_RESULT_OK)
    {
        ESP_LOGE(TAG,
                 "SIM transport initialization FAILED: %d",
                 result);
        return result;
    }

    tx_packet[sizeof(tx_packet) - 1u] =
        gwPacketCrc8(
            tx_packet,
            sizeof(tx_packet) - 1u);

    result = gwTransportSend(
        tx_packet,
        sizeof(tx_packet));

    if (result != GW_RESULT_OK)
    {
        ESP_LOGE(TAG,
                 "SIM transport send FAILED: %d",
                 result);
        (void)gwTransportDeinit();
        return result;
    }

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
        ESP_LOGE(TAG,
                 "SIM transport receive FAILED: %d",
                 result);
        (void)gwTransportDeinit();
        return result;
    }

    if (received_length != sizeof(tx_packet))
    {
        ESP_LOGE(TAG,
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

    result = gwTransportDeinit();

    if (result != GW_RESULT_OK)
    {
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

    ack.node = GW_SIM_TEST_NODE_ID;
    ack.type = GW_PKT_ACK;
    ack.seq = 1u;
    ack.result = GW_ACK_OK;
    ack.gwid = GW_SIM_TEST_GATEWAY_ID;

    ack.crc8 = gwPacketCrc8(
        (const uint8_t *)&ack,
        sizeof(ack) - 1u);

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
/* Invalid STATUS packet tests                                                */
/* -------------------------------------------------------------------------- */

static GwResult_t gwSimTestInvalidStatus(void)
{
    GwLoRaStatus_t status = {0};
    GwEventMessage_t message;
    GwResult_t result;

    status.node = GW_SIM_TEST_NODE_ID;
    status.type = GW_PKT_STATUS;
    status.seq = 4u;
    status.motor_state = 1u;
    status.fault_flags = 0u;
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

    result = gwEventServiceProcess(
        &message);

    if (result != GW_RESULT_OK)
    {
        ESP_LOGE(
            TAG,
            "[FAIL] Valid STATUS baseline");

        return result;
    }

    ESP_LOGI(
        TAG,
        "[PASS] Valid STATUS baseline");

    message.length = sizeof(status) - 1u;

    result = gwEventServiceProcess(
        &message);

    if (result != GW_RESULT_INVALID_ARG)
    {
        ESP_LOGE(
            TAG,
            "[FAIL] STATUS wrong length accepted: %d",
            result);

        return GW_RESULT_ERROR;
    }

    ESP_LOGI(
        TAG,
        "[PASS] STATUS wrong length rejected");

    message.length = sizeof(status);
    status.type = GW_PKT_EVENT;

    status.crc8 = gwPacketCrc8(
        (const uint8_t *)&status,
        sizeof(status) - 1u);

    result = gwEventServiceProcess(
        &message);

    if (result != GW_RESULT_INVALID_ARG)
    {
        ESP_LOGE(
            TAG,
            "[FAIL] STATUS wrong type accepted: %d",
            result);

        return GW_RESULT_ERROR;
    }

    ESP_LOGI(
        TAG,
        "[PASS] STATUS wrong type rejected");

    status.type = GW_PKT_STATUS;

    status.crc8 = gwPacketCrc8(
        (const uint8_t *)&status,
        sizeof(status) - 1u);

    status.crc8 ^= 0x01u;

    message.length = sizeof(status);

    result = gwEventServiceProcess(
        &message);

    if (result != GW_RESULT_ERROR)
    {
        ESP_LOGE(
            TAG,
            "[FAIL] STATUS bad CRC accepted: %d",
            result);

        return GW_RESULT_ERROR;
    }

    ESP_LOGI(
        TAG,
        "[PASS] STATUS bad CRC rejected");

    return GW_RESULT_OK;
}

/* -------------------------------------------------------------------------- */
/* Invalid EVENT packet tests                                                 */
/* -------------------------------------------------------------------------- */

static GwResult_t gwSimTestInvalidEvent(void)
{
    GwLoRaEvent_t event = {0};
    GwEventMessage_t message;
    GwResult_t result;

    event.node = GW_SIM_TEST_NODE_ID;
    event.type = GW_PKT_EVENT;
    event.seq = 5u;
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

    result = gwEventServiceProcess(
        &message);

    if (result != GW_RESULT_OK)
    {
        ESP_LOGE(
            TAG,
            "[FAIL] Valid EVENT baseline");

        return result;
    }

    ESP_LOGI(
        TAG,
        "[PASS] Valid EVENT baseline");

    message.length = sizeof(event) - 1u;

    result = gwEventServiceProcess(
        &message);

    if (result != GW_RESULT_INVALID_ARG)
    {
        ESP_LOGE(
            TAG,
            "[FAIL] EVENT wrong length accepted: %d",
            result);

        return GW_RESULT_ERROR;
    }

    ESP_LOGI(
        TAG,
        "[PASS] EVENT wrong length rejected");

    message.length = sizeof(event);
    event.type = GW_PKT_STATUS;

    event.crc8 = gwPacketCrc8(
        (const uint8_t *)&event,
        sizeof(event) - 1u);

    result = gwEventServiceProcess(
        &message);

    if (result != GW_RESULT_INVALID_ARG)
    {
        ESP_LOGE(
            TAG,
            "[FAIL] EVENT wrong type accepted: %d",
            result);

        return GW_RESULT_ERROR;
    }

    ESP_LOGI(
        TAG,
        "[PASS] EVENT wrong type rejected");

    event.type = GW_PKT_EVENT;

    event.crc8 = gwPacketCrc8(
        (const uint8_t *)&event,
        sizeof(event) - 1u);

    event.crc8 ^= 0x01u;

    message.length = sizeof(event);

    result = gwEventServiceProcess(
        &message);

    if (result != GW_RESULT_ERROR)
    {
        ESP_LOGE(
            TAG,
            "[FAIL] EVENT bad CRC accepted: %d",
            result);

        return GW_RESULT_ERROR;
    }

    ESP_LOGI(
        TAG,
        "[PASS] EVENT bad CRC rejected");

    return GW_RESULT_OK;
}

/* -------------------------------------------------------------------------- */
/* Unregistered node rejection tests                                          */
/* -------------------------------------------------------------------------- */

static GwResult_t gwSimTestUnregisteredNode(void)
{
    const uint8_t unregistered_node = 0x02u;

    GwLoRaStatus_t status = {0};
    GwLoRaEvent_t event = {0};

    GwEventMessage_t message;

    GwResult_t result;

    /* ---------------------------------------------------------------------- */
    /* Unregistered STATUS                                                    */
    /* ---------------------------------------------------------------------- */

    status.node = unregistered_node;
    status.type = GW_PKT_STATUS;
    status.seq = 6u;
    status.motor_state = 1u;
    status.fault_flags = 0u;
    status.turns100 = 1000u;
    status.voltage100 = 2400u;
    status.current100 = 300u;
    status.rssi = -60;
    status.gwid = GW_SIM_TEST_GATEWAY_ID;

    status.crc8 = gwPacketCrc8(
        (const uint8_t *)&status,
        sizeof(status) - 1u);

    message.type = GW_EVENT_TYPE_STATUS;
    message.data = (const uint8_t *)&status;
    message.length = sizeof(status);

    result = gwEventServiceProcess(
        &message);

    if (result == GW_RESULT_OK)
    {
        ESP_LOGE(
            TAG,
            "[FAIL] Unregistered STATUS was accepted");

        return GW_RESULT_ERROR;
    }

    ESP_LOGI(
        TAG,
        "[PASS] Unregistered STATUS rejected");

    /* ---------------------------------------------------------------------- */
    /* Unregistered EVENT                                                     */
    /* ---------------------------------------------------------------------- */

    event.node = unregistered_node;
    event.type = GW_PKT_EVENT;
    event.seq = 7u;
    event.event = GW_EVT_OPEN_DONE;
    event.voltage100 = 2400u;
    event.current100 = 300u;
    event.gwid = GW_SIM_TEST_GATEWAY_ID;

    event.crc8 = gwPacketCrc8(
        (const uint8_t *)&event,
        sizeof(event) - 1u);

    message.type = GW_EVENT_TYPE_EVENT;
    message.data = (const uint8_t *)&event;
    message.length = sizeof(event);

    result = gwEventServiceProcess(
        &message);

    if (result == GW_RESULT_OK)
    {
        ESP_LOGE(
            TAG,
            "[FAIL] Unregistered EVENT was accepted");

        return GW_RESULT_ERROR;
    }

    ESP_LOGI(
        TAG,
        "[PASS] Unregistered EVENT rejected");

    return GW_RESULT_OK;
}

/* -------------------------------------------------------------------------- */
/* Schedule STATUS validation                                                */
/* -------------------------------------------------------------------------- */

static GwResult_t gwSimTestScheduleStatus(void)
{
    GwLoRaSchedStatus_t packet = {0};
    GwEventMessage_t message;
    GwNodeInfo_t info;
    GwResult_t result;

    /* ---------------------------------------------------------------------- */
    /* Valid schedule status                                                  */
    /* ---------------------------------------------------------------------- */

    packet.node = GW_SIM_TEST_NODE_ID;
    packet.type = GW_PKT_SCHED;
    packet.seq = 8u;
    packet.slot = 0u;
    packet.enabled = 1u;
    packet.action = GW_CMD_OPEN;
    packet.hour = 6u;
    packet.minute = 30u;
    packet.days = 0x7Fu;
    packet.gwid = GW_SIM_TEST_GATEWAY_ID;

    packet.crc8 = gwPacketCrc8(
        (const uint8_t *)&packet,
        sizeof(packet) - 1u);

    message.type = GW_EVENT_TYPE_SCHEDULE_STATUS;
    message.data = (const uint8_t *)&packet;
    message.length = sizeof(packet);

    result = gwEventServiceProcess(
        &message);

    if (result != GW_RESULT_OK)
    {
        ESP_LOGE(
            TAG,
            "[FAIL] Valid SCHEDULE STATUS: %d",
            result);

        return result;
    }

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
            "[FAIL] SCHEDULE STATUS did not set node ONLINE");

        return GW_RESULT_ERROR;
    }

    ESP_LOGI(
        TAG,
        "[PASS] Valid SCHEDULE STATUS");

    /* ---------------------------------------------------------------------- */
    /* Wrong length                                                           */
    /* ---------------------------------------------------------------------- */

    message.length = sizeof(packet) - 1u;

    result = gwEventServiceProcess(
        &message);

    if (result != GW_RESULT_INVALID_ARG)
    {
        ESP_LOGE(
            TAG,
            "[FAIL] SCHEDULE wrong length accepted: %d",
            result);

        return GW_RESULT_ERROR;
    }

    ESP_LOGI(
        TAG,
        "[PASS] SCHEDULE wrong length rejected");

    /* ---------------------------------------------------------------------- */
    /* Wrong packet type                                                      */
    /* ---------------------------------------------------------------------- */

    message.length = sizeof(packet);
    packet.type = GW_PKT_EVENT;

    packet.crc8 = gwPacketCrc8(
        (const uint8_t *)&packet,
        sizeof(packet) - 1u);

    result = gwEventServiceProcess(
        &message);

    if (result != GW_RESULT_INVALID_ARG)
    {
        ESP_LOGE(
            TAG,
            "[FAIL] SCHEDULE wrong type accepted: %d",
            result);

        return GW_RESULT_ERROR;
    }

    ESP_LOGI(
        TAG,
        "[PASS] SCHEDULE wrong type rejected");

    /* ---------------------------------------------------------------------- */
    /* Bad CRC                                                                */
    /* ---------------------------------------------------------------------- */

    packet.type = GW_PKT_SCHED;

    packet.crc8 = gwPacketCrc8(
        (const uint8_t *)&packet,
        sizeof(packet) - 1u);

    packet.crc8 ^= 0x01u;

    result = gwEventServiceProcess(
        &message);

    if (result != GW_RESULT_ERROR)
    {
        ESP_LOGE(
            TAG,
            "[FAIL] SCHEDULE bad CRC accepted: %d",
            result);

        return GW_RESULT_ERROR;
    }

    ESP_LOGI(
        TAG,
        "[PASS] SCHEDULE bad CRC rejected");

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
/* Full Gateway SIM test                                                      */
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
    /* Transport                                                              */
    /* ---------------------------------------------------------------------- */

    result = gwSimTestTransportLoopback();

    if (result != GW_RESULT_OK)
    {
        return result;
    }

    /* ---------------------------------------------------------------------- */
    /* Command Service                                                        */
    /* ---------------------------------------------------------------------- */

    result = gwSimTestCommandService();

    if (result != GW_RESULT_OK)
    {
        return result;
    }

    /* ---------------------------------------------------------------------- */
    /* ACK                                                                     */
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
    /* Register test node                                                     */
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
    /* STATUS + EVENT                                                         */
    /* ---------------------------------------------------------------------- */

    result = gwSimTestStatusEvent();

    if (result != GW_RESULT_OK)
    {
        return result;
    }

    /* ---------------------------------------------------------------------- */
    /* Invalid STATUS                                                         */
    /* ---------------------------------------------------------------------- */

    result = gwSimTestInvalidStatus();

    if (result != GW_RESULT_OK)
    {
        return result;
    }

    /* ---------------------------------------------------------------------- */
    /* Invalid EVENT                                                          */
    /* ---------------------------------------------------------------------- */

    result = gwSimTestInvalidEvent();

    if (result != GW_RESULT_OK)
    {
        return result;
    }

    /* ---------------------------------------------------------------------- */
    /* Unregistered node                                                      */
    /* ---------------------------------------------------------------------- */

       result = gwSimTestUnregisteredNode();

    if (result != GW_RESULT_OK)
    {
        return result;
    }

    /* ---------------------------------------------------------------------- */
    /* Schedule STATUS                                                        */
    /* ---------------------------------------------------------------------- */

    result = gwSimTestScheduleStatus();

    if (result != GW_RESULT_OK)
    {
        return result;
    }

    /* ---------------------------------------------------------------------- */
    /* Fault handling                                                         */
    /* ---------------------------------------------------------------------- */

    result = gwSimTestFaultStatus();

    if (result != GW_RESULT_OK)
    {
        return result;
    }

    /* ---------------------------------------------------------------------- */
    /* Complete                                                               */
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