#include "gw_eol_manager.h"

#include "gw_transport.h"

#include <string.h>

static bool s_initialized = false;
static bool s_active = false;

static uint8_t s_node = 0u;
static uint8_t s_gwid = GW_EOL_FACTORY_GW_ID;
static uint8_t s_sequence = 0u;

static uint8_t s_pass_count = 0u;
static uint8_t s_fail_count = 0u;

static uint8_t gwEolNextSequence(void)
{
    const uint8_t sequence = s_sequence;
    s_sequence++;

    return sequence;
}

static void gwEolPrepareCrc(
    uint8_t *data,
    size_t length)
{
    data[length - 1u] =
        gwEolCrc8(data, length - 1u);
}

GwResult_t gwEolManagerInit(void)
{
    if (s_initialized)
    {
        return GW_RESULT_ALREADY_INITIALIZED;
    }

    s_active = false;
    s_node = 0u;
    s_gwid = GW_EOL_FACTORY_GW_ID;
    s_sequence = 0u;
    s_pass_count = 0u;
    s_fail_count = 0u;

    s_initialized = true;

    return GW_RESULT_OK;
}

GwResult_t gwEolManagerDeinit(void)
{
    if (!s_initialized)
    {
        return GW_RESULT_NOT_INITIALIZED;
    }

    s_active = false;
    s_initialized = false;

    return GW_RESULT_OK;
}

bool gwEolManagerIsInitialized(void)
{
    return s_initialized;
}

GwResult_t gwEolManagerStart(
    uint8_t node,
    uint8_t gwid)
{
    GwEolStart_t packet;

    if (!s_initialized)
    {
        return GW_RESULT_NOT_INITIALIZED;
    }

    if (s_active)
    {
        return GW_RESULT_BUSY;
    }

    if (node == 0u)
    {
        return GW_RESULT_INVALID_ARG;
    }

    memset(&packet, 0, sizeof(packet));

    s_node = node;
    s_gwid = gwid;
    s_pass_count = 0u;
    s_fail_count = 0u;

    packet.node = s_node;
    packet.type = GW_EOL_PKT_START;
    packet.seq = gwEolNextSequence();
    packet.total_tests = GW_EOL_TOTAL_TESTS;
    packet.fw_major = 0u;
    packet.fw_minor = 0u;
    packet.gwid = s_gwid;

    gwEolPrepareCrc(
        (uint8_t *)&packet,
        sizeof(packet));

    {
        const GwResult_t result =
            gwTransportSend(
                (const uint8_t *)&packet,
                sizeof(packet));

        if (result != GW_RESULT_OK)
        {
            return result;
        }
    }

    s_active = true;

    return GW_RESULT_OK;
}

GwResult_t gwEolManagerCancel(void)
{
    GwEolTestRequest_t packet;

    if (!s_initialized)
    {
        return GW_RESULT_NOT_INITIALIZED;
    }

    if (!s_active)
    {
        return GW_RESULT_INVALID_ARG;
    }

    memset(&packet, 0, sizeof(packet));

    packet.node = s_node;
    packet.type = GW_EOL_PKT_CANCEL;
    packet.seq = gwEolNextSequence();
    packet.test_idx = 0u;
    packet.gwid = s_gwid;

    gwEolPrepareCrc(
        (uint8_t *)&packet,
        sizeof(packet));

    {
        const GwResult_t result =
            gwTransportSend(
                (const uint8_t *)&packet,
                sizeof(packet));

        if (result != GW_RESULT_OK)
        {
            return result;
        }
    }

    s_active = false;

    return GW_RESULT_OK;
}

GwResult_t gwEolManagerRequestTest(
    uint8_t test_idx)
{
    GwEolTestRequest_t packet;

    if (!s_initialized)
    {
        return GW_RESULT_NOT_INITIALIZED;
    }

    if (!s_active)
    {
        return GW_RESULT_NOT_INITIALIZED;
    }

    if (test_idx >= GW_EOL_TOTAL_TESTS)
    {
        return GW_RESULT_INVALID_ARG;
    }

    memset(&packet, 0, sizeof(packet));

    packet.node = s_node;
    packet.type = GW_EOL_PKT_TEST_REQUEST;
    packet.seq = gwEolNextSequence();
    packet.test_idx = test_idx;
    packet.gwid = s_gwid;

    gwEolPrepareCrc(
        (uint8_t *)&packet,
        sizeof(packet));

    return gwTransportSend(
        (const uint8_t *)&packet,
        sizeof(packet));
}

GwResult_t gwEolManagerProcessRx(
    const uint8_t *data,
    size_t length)
{
    if (!s_initialized)
    {
        return GW_RESULT_NOT_INITIALIZED;
    }

    if (data == NULL || length < 2u)
    {
        return GW_RESULT_INVALID_ARG;
    }

    if (!s_active)
    {
        return GW_RESULT_INVALID_ARG;
    }

    switch (data[1])
    {
        case GW_EOL_PKT_RESULT:
        {
            GwEolResult_t packet;

            if (!gwEolValidatePacket(
                    data,
                    length,
                    GW_EOL_PKT_RESULT))
            {
                return GW_RESULT_ERROR;
            }

            memcpy(
                &packet,
                data,
                sizeof(packet));

            if (packet.node != s_node)
            {
                return GW_RESULT_INVALID_ARG;
            }

            if (packet.pass != 0u)
            {
                s_pass_count++;
            }
            else
            {
                s_fail_count++;
            }

            return GW_RESULT_OK;
        }

        case GW_EOL_PKT_SUMMARY:
        {
            GwEolSummary_t packet;

            if (!gwEolValidatePacket(
                    data,
                    length,
                    GW_EOL_PKT_SUMMARY))
            {
                return GW_RESULT_ERROR;
            }

            memcpy(
                &packet,
                data,
                sizeof(packet));

            if (packet.node != s_node)
            {
                return GW_RESULT_INVALID_ARG;
            }

            s_pass_count = packet.pass_count;
            s_fail_count = packet.fail_count;
            s_active = false;

            return GW_RESULT_OK;
        }

        case GW_EOL_PKT_ACK:
        {
            GwEolAck_t packet;

            if (!gwEolValidatePacket(
                    data,
                    length,
                    GW_EOL_PKT_ACK))
            {
                return GW_RESULT_ERROR;
            }

            memcpy(
                &packet,
                data,
                sizeof(packet));

            if (packet.node != s_node)
            {
                return GW_RESULT_INVALID_ARG;
            }

            if (packet.result == GW_EOL_ACK_CANCEL)
            {
                s_active = false;
            }

            return GW_RESULT_OK;
        }

        default:
            return GW_RESULT_INVALID_ARG;
    }
}

bool gwEolManagerIsActive(void)
{
    return s_active;
}

uint8_t gwEolManagerGetNode(void)
{
    return s_node;
}

uint8_t gwEolManagerGetSequence(void)
{
    return s_sequence;
}

uint8_t gwEolManagerGetPassCount(void)
{
    return s_pass_count;
}

uint8_t gwEolManagerGetFailCount(void)
{
    return s_fail_count;
}
