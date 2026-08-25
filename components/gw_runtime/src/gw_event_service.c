#include "gw_event_service.h"

#include "gw_node_manager.h"
#include "gw_protocol.h"

#include <string.h>

static bool s_initialized = false;

/* -------------------------------------------------------------------------- */
/* Internal packet validation                                                  */
/* -------------------------------------------------------------------------- */

static GwResult_t gwEventValidatePacket(
    const uint8_t *data,
    size_t length,
    uint8_t expected_type,
    size_t expected_size)
{
    if (data == NULL)
    {
        return GW_RESULT_INVALID_ARG;
    }

    if (length != expected_size)
    {
        return GW_RESULT_INVALID_ARG;
    }

    if (data[1] != expected_type)
    {
        return GW_RESULT_INVALID_ARG;
    }

    if (!gwPacketCrcValid(data, length))
    {
        return GW_RESULT_ERROR;
    }

    return GW_RESULT_OK;
}

/* -------------------------------------------------------------------------- */
/* ACK                                                                         */
/* -------------------------------------------------------------------------- */

static GwResult_t gwEventProcessAck(
    const uint8_t *data,
    size_t length)
{
    const GwLoRaAck_t *packet;
    GwResult_t result;

    result = gwEventValidatePacket(
        data,
        length,
        GW_PKT_ACK,
        sizeof(GwLoRaAck_t));

    if (result != GW_RESULT_OK)
    {
        return result;
    }

    packet = (const GwLoRaAck_t *)data;

    /*
     * ACK processing will later be connected to the
     * command transaction / sequence manager.
     *
     * For this Layer-3 revision, packet validation is
     * sufficient.
     */
    (void)packet;

    return GW_RESULT_OK;
}

/* -------------------------------------------------------------------------- */
/* STATUS                                                                      */
/* -------------------------------------------------------------------------- */

static GwResult_t gwEventProcessStatus(
    const uint8_t *data,
    size_t length)
{
    const GwLoRaStatus_t *packet;
    GwNodeTelemetry_t telemetry;
    GwResult_t result;

    result = gwEventValidatePacket(
        data,
        length,
        GW_PKT_STATUS,
        sizeof(GwLoRaStatus_t));

    if (result != GW_RESULT_OK)
    {
        return result;
    }

    packet = (const GwLoRaStatus_t *)data;

    /*
     * A status packet from an unregistered node must not
     * silently create a runtime node entry.
     */
    if (!gwNodeManagerIsRegistered(packet->node))
    {
        return GW_RESULT_ERROR;
    }

    memset(
        &telemetry,
        0,
        sizeof(telemetry));

    telemetry.motor_state = packet->motor_state;
    telemetry.fault_flags = packet->fault_flags;
    telemetry.turns100 = packet->turns100;
    telemetry.voltage100 = packet->voltage100;
    telemetry.current100 = packet->current100;
    telemetry.rssi = packet->rssi;
    telemetry.valid = true;

    result = gwNodeManagerUpdateTelemetry(
        packet->node,
        &telemetry);

    if (result != GW_RESULT_OK)
    {
        return result;
    }

    /*
     * Valid telemetry means the Node is currently reachable.
     */
    if (packet->fault_flags != 0u)
    {
        return gwNodeManagerSetState(
            packet->node,
            GW_NODE_STATE_FAULT);
    }

    return gwNodeManagerSetState(
        packet->node,
        GW_NODE_STATE_ONLINE);
}

/* -------------------------------------------------------------------------- */
/* EVENT                                                                       */
/* -------------------------------------------------------------------------- */

static GwResult_t gwEventProcessEvent(
    const uint8_t *data,
    size_t length)
{
    const GwLoRaEvent_t *packet;
    GwNodeTelemetry_t telemetry;
    GwResult_t result;

    result = gwEventValidatePacket(
        data,
        length,
        GW_PKT_EVENT,
        sizeof(GwLoRaEvent_t));

    if (result != GW_RESULT_OK)
    {
        return result;
    }

    packet = (const GwLoRaEvent_t *)data;

    if (!gwNodeManagerIsRegistered(packet->node))
    {
        return GW_RESULT_ERROR;
    }

    /*
     * EVENT packets carry instantaneous voltage/current
     * information. Preserve that information in the
     * Gateway runtime telemetry model.
     *
     * Existing telemetry fields that are not present in
     * the event packet are intentionally left unchanged.
     */
    if (gwNodeManagerGetTelemetry(
            packet->node,
            &telemetry) == GW_RESULT_OK)
    {
        telemetry.voltage100 = packet->voltage100;
        telemetry.current100 = packet->current100;
        telemetry.valid = true;

        result = gwNodeManagerUpdateTelemetry(
            packet->node,
            &telemetry);

        if (result != GW_RESULT_OK)
        {
            return result;
        }
    }

    /*
     * Fault event is a runtime fault condition.
     *
     * Do not mark a Node ONLINE merely because the packet
     * itself was successfully received.
     */
    if (packet->event == GW_EVT_FAULT ||
        packet->event == GW_EVT_CALIB_FAIL)
    {
        return gwNodeManagerSetState(
            packet->node,
            GW_NODE_STATE_FAULT);
    }

    /*
     * A valid operational event confirms that the Node is
     * reachable.
     *
     * EOL/manufacturing events are deliberately not given
     * special production-runtime behavior here.
     */
    switch (packet->event)
    {
        case GW_EVT_OPEN_DONE:
        case GW_EVT_CLOSE_DONE:
        case GW_EVT_LOW_VOLTAGE:
        case GW_EVT_BOOT:
        case GW_EVT_WAIT_BYPASS:
        case GW_EVT_BOUND:
        case GW_EVT_REBIND:
        case GW_EVT_CALIB_DONE:
        case GW_EVT_CALIB_STEP:
        case GW_EVT_REBIND_COMPLETE:
            return gwNodeManagerSetState(
                packet->node,
                GW_NODE_STATE_ONLINE);

        case GW_EVT_NOT_EOL_TESTED:
            /*
             * This is an EOL/manufacturing indication.
             *
             * Production runtime must not introduce EOL
             * behavior or state transitions here.
             */
            return GW_RESULT_OK;

        default:
            return GW_RESULT_INVALID_ARG;
    }
}

/* -------------------------------------------------------------------------- */
/* Schedule status                                                             */
/* -------------------------------------------------------------------------- */

static GwResult_t gwEventProcessScheduleStatus(
    const uint8_t *data,
    size_t length)
{
    const GwLoRaSchedStatus_t *packet;
    GwResult_t result;

    result = gwEventValidatePacket(
        data,
        length,
        GW_PKT_SCHED,
        sizeof(GwLoRaSchedStatus_t));

    if (result != GW_RESULT_OK)
    {
        return result;
    }

    packet = (const GwLoRaSchedStatus_t *)data;

    if (!gwNodeManagerIsRegistered(packet->node))
    {
        return GW_RESULT_ERROR;
    }

    /*
     * Schedule state storage is not yet part of the
     * Node Manager runtime model.
     *
     * Validate the packet and confirm Node reachability.
     */
    return gwNodeManagerSetState(
        packet->node,
        GW_NODE_STATE_ONLINE);
}

/* -------------------------------------------------------------------------- */
/* Rebind event                                                                */
/* -------------------------------------------------------------------------- */

static GwResult_t gwEventProcessRebind(
    const uint8_t *data,
    size_t length)
{
    const GwLoRaRebindEvent_t *packet;
    GwResult_t result;

    /*
     * Rebind is represented on the wire as a normal
     * GW_PKT_EVENT packet.
     */
    result = gwEventValidatePacket(
        data,
        length,
        GW_PKT_EVENT,
        sizeof(GwLoRaRebindEvent_t));

    if (result != GW_RESULT_OK)
    {
        return result;
    }

    packet = (const GwLoRaRebindEvent_t *)data;

    /*
     * Rebind handling is deliberately kept separate from
     * normal telemetry registration.
     *
     * Persistent ownership changes require the dedicated
     * rebind service.
     */
    (void)packet;

    return GW_RESULT_OK;
}

/* -------------------------------------------------------------------------- */
/* Lifecycle                                                                   */
/* -------------------------------------------------------------------------- */

GwResult_t gwEventServiceInit(void)
{
    if (s_initialized)
    {
        return GW_RESULT_ALREADY_INITIALIZED;
    }

    s_initialized = true;

    return GW_RESULT_OK;
}

GwResult_t gwEventServiceDeinit(void)
{
    if (!s_initialized)
    {
        return GW_RESULT_NOT_INITIALIZED;
    }

    s_initialized = false;

    return GW_RESULT_OK;
}

/* -------------------------------------------------------------------------- */
/* Public processing API                                                       */
/* -------------------------------------------------------------------------- */

GwResult_t gwEventServiceProcess(
    const GwEventMessage_t *message)
{
    if (!s_initialized)
    {
        return GW_RESULT_NOT_INITIALIZED;
    }

    if (message == NULL)
    {
        return GW_RESULT_INVALID_ARG;
    }

    if (message->data == NULL ||
        message->length == 0u)
    {
        return GW_RESULT_INVALID_ARG;
    }

    switch (message->type)
    {
        case GW_EVENT_TYPE_ACK:
            return gwEventProcessAck(
                message->data,
                message->length);

        case GW_EVENT_TYPE_STATUS:
            return gwEventProcessStatus(
                message->data,
                message->length);

        case GW_EVENT_TYPE_EVENT:
            return gwEventProcessEvent(
                message->data,
                message->length);

        case GW_EVENT_TYPE_SCHEDULE_STATUS:
            return gwEventProcessScheduleStatus(
                message->data,
                message->length);

        case GW_EVENT_TYPE_REBIND:
            return gwEventProcessRebind(
                message->data,
                message->length);

        default:
            return GW_RESULT_INVALID_ARG;
    }
}

/* -------------------------------------------------------------------------- */
/* Queries                                                                     */
/* -------------------------------------------------------------------------- */

bool gwEventServiceIsInitialized(void)
{
    return s_initialized;
}