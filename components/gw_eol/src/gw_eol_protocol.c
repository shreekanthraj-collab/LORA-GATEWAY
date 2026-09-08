#include "gw_eol_protocol.h"

uint8_t gwEolCrc8(
    const uint8_t *data,
    size_t length)
{
    uint8_t crc = 0x00u;

    if (data == NULL)
    {
        return 0u;
    }

    for (size_t i = 0u; i < length; ++i)
    {
        crc ^= data[i];

        for (uint8_t bit = 0u; bit < 8u; ++bit)
        {
            if ((crc & 0x80u) != 0u)
            {
                crc = (uint8_t)(
                    (crc << 1u) ^
                    GW_EOL_CRC8_POLY);
            }
            else
            {
                crc <<= 1u;
            }
        }
    }

    return crc;
}

bool gwEolValidatePacket(
    const uint8_t *data,
    size_t length,
    uint8_t expected_type)
{
    size_t expected_length;

    if (data == NULL)
    {
        return false;
    }

    switch (expected_type)
    {
        case GW_EOL_PKT_START:
            expected_length = GW_EOL_START_SIZE;
            break;

        case GW_EOL_PKT_RESULT:
            expected_length = GW_EOL_RESULT_SIZE;
            break;

        case GW_EOL_PKT_SUMMARY:
            expected_length = GW_EOL_SUMMARY_SIZE;
            break;

        case GW_EOL_PKT_ACK:
            expected_length = GW_EOL_ACK_SIZE;
            break;

        case GW_EOL_PKT_TEST_REQUEST:
            expected_length = GW_EOL_TEST_REQUEST_SIZE;
            break;

        default:
            return false;
    }

    if (length != expected_length)
    {
        return false;
    }

    if (data[1] != expected_type)
    {
        return false;
    }

    return data[length - 1u] ==
           gwEolCrc8(data, length - 1u);
}
