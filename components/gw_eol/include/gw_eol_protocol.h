#ifndef GW_EOL_PROTOCOL_H
#define GW_EOL_PROTOCOL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GW_EOL_PKT_START        0x60u
#define GW_EOL_PKT_RESULT       0x61u
#define GW_EOL_PKT_SUMMARY      0x62u
#define GW_EOL_PKT_ACK          0x63u
#define GW_EOL_PKT_CANCEL       0x64u
#define GW_EOL_PKT_TEST_REQUEST 0x65u

#define GW_EOL_NAME_LEN         12u
#define GW_EOL_TOTAL_TESTS      16u
#define GW_EOL_FACTORY_GW_ID    0x00u

#define GW_EOL_ACK_OK            0x01u
#define GW_EOL_ACK_RETRY         0x02u
#define GW_EOL_ACK_CANCEL        0x03u

#define GW_EOL_START_SIZE        8u
#define GW_EOL_RESULT_SIZE       19u
#define GW_EOL_SUMMARY_SIZE      9u
#define GW_EOL_ACK_SIZE          6u
#define GW_EOL_TEST_REQUEST_SIZE 6u

#define GW_EOL_CRC8_POLY         0x31u

#pragma pack(push, 1)

typedef struct
{
    uint8_t node;
    uint8_t type;
    uint8_t seq;
    uint8_t total_tests;
    uint8_t fw_major;
    uint8_t fw_minor;
    uint8_t gwid;
    uint8_t crc8;
} GwEolStart_t;

typedef struct
{
    uint8_t node;
    uint8_t type;
    uint8_t seq;
    uint8_t test_idx;
    uint8_t pass;
    char    name[GW_EOL_NAME_LEN];
    uint8_t gwid;
    uint8_t crc8;
} GwEolResult_t;

typedef struct
{
    uint8_t node;
    uint8_t type;
    uint8_t seq;
    uint8_t overall;
    uint8_t pass_count;
    uint8_t fail_count;
    uint8_t total;
    uint8_t gwid;
    uint8_t crc8;
} GwEolSummary_t;

typedef struct
{
    uint8_t node;
    uint8_t type;
    uint8_t seq;
    uint8_t result;
    uint8_t gwid;
    uint8_t crc8;
} GwEolAck_t;

typedef struct
{
    uint8_t node;
    uint8_t type;
    uint8_t seq;
    uint8_t test_idx;
    uint8_t gwid;
    uint8_t crc8;
} GwEolTestRequest_t;

#pragma pack(pop)

_Static_assert(sizeof(GwEolStart_t) == GW_EOL_START_SIZE,
               "GwEolStart_t size mismatch");

_Static_assert(sizeof(GwEolResult_t) == GW_EOL_RESULT_SIZE,
               "GwEolResult_t size mismatch");

_Static_assert(sizeof(GwEolSummary_t) == GW_EOL_SUMMARY_SIZE,
               "GwEolSummary_t size mismatch");

_Static_assert(sizeof(GwEolAck_t) == GW_EOL_ACK_SIZE,
               "GwEolAck_t size mismatch");

_Static_assert(sizeof(GwEolTestRequest_t) == GW_EOL_TEST_REQUEST_SIZE,
               "GwEolTestRequest_t size mismatch");

uint8_t gwEolCrc8(
    const uint8_t *data,
    size_t length);

bool gwEolValidatePacket(
    const uint8_t *data,
    size_t length,
    uint8_t expected_type);

#ifdef __cplusplus
}
#endif

#endif /* GW_EOL_PROTOCOL_H */
