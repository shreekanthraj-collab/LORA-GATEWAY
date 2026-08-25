#include "gw_protocol.h"

uint8_t gwPacketCrc8(
    const uint8_t *data,
    size_t length)
{
    uint8_t crc = 0x00u;

    if ((data == NULL) && (length != 0u))
    {
        return 0u;
    }

    for (size_t i = 0U;
         i < length;
         ++i)
    {
        crc ^= data[i];

        for (uint8_t bit = 0U;
             bit < 8U;
             ++bit)
        {
            if ((crc & 0x80U) != 0U)
            {
                crc = (uint8_t)(
                    (crc << 1U) ^
                    GW_PACKET_CRC8_POLY);
            }
            else
            {
                crc <<= 1U;
            }
        }
    }

    return crc;
}

uint8_t gwRebindAuthCrc8(
    uint8_t current_gwid,
    uint8_t new_gwid,
    uint8_t seq,
    uint8_t rebind_secret)
{
    const uint8_t data[4] = {
        current_gwid,
        new_gwid,
        seq,
        rebind_secret
    };

    uint8_t crc = 0x00u;

    for (size_t i = 0U;
         i < sizeof(data);
         ++i)
    {
        crc ^= data[i];

        for (uint8_t bit = 0U;
             bit < 8U;
             ++bit)
        {
            if ((crc & 0x80U) != 0U)
            {
                crc = (uint8_t)(
                    (crc << 1U) ^
                    GW_REBIND_CRC8_POLY);
            }
            else
            {
                crc <<= 1U;
            }
        }
    }

    return crc;
}

bool gwPacketCrcValid(
    const uint8_t *packet,
    size_t length)
{
    if ((packet == NULL) ||
        (length < 2U))
    {
        return false;
    }

    return gwPacketCrc8(
               packet,
               length - 1U) ==
           packet[length - 1U];
}