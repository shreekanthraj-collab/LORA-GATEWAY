#ifndef GW_PROTOCOL_H
#define GW_PROTOCOL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Packet types */
#define GW_PKT_CMD       0x10u
#define GW_PKT_ACK       0xAAu
#define GW_PKT_STATUS    0x20u
#define GW_PKT_EVENT     0x30u
#define GW_PKT_SCHED     0x40u

/* Gateway IDs */
#define GWID_UNBOUND     0x00u
#define GWID_RESERVED    0xFFu
#define GWID_AGRI_MIN    0x01u
#define GWID_AGRI_MAX    0x7Fu
#define GWID_MUNI_MIN    0x80u
#define GWID_MUNI_MAX    0xFEu

/* Commands */
#define GW_CMD_OPEN                   0x01u
#define GW_CMD_CLOSE                  0x02u
#define GW_CMD_STOP                   0x03u
#define GW_CMD_GET_STATUS             0x04u
#define GW_CMD_CLEAR_FAULT            0x05u
#define GW_CMD_SET_TURNS              0x06u
#define GW_CMD_SET_SCHEDULE           0x07u
#define GW_CMD_CLR_SCHEDULE           0x08u
#define GW_CMD_SET_CHANNEL            0x09u
#define GW_CMD_VOLTAGE_BYPASS         0x0Au
#define GW_CMD_VOLTAGE_CANCEL         0x0Bu
#define GW_CMD_SET_TIME               0x0Cu
#define GW_CMD_GET_SCHEDULE           0x0Du
#define GW_CMD_SET_CURRENT            0x0Eu
#define GW_CMD_CALIBRATE              0x0Fu
#define GW_CMD_CAL_SET                0x10u
#define GW_CMD_CAL_ABORT              0x11u
#define GW_CMD_ENTER_EOL              0x12u
#define GW_CMD_REBIND_OWNER           0x13u
#define GW_CMD_SET_DISENGAGE_CURRENT  0x15u

/* ACK results */
#define GW_ACK_OK       0x01u
#define GW_ACK_DENIED   0x02u
#define GW_ACK_FAULT    0x03u

/* Events */
#define GW_EVT_OPEN_DONE        0x01u
#define GW_EVT_CLOSE_DONE       0x02u
#define GW_EVT_FAULT            0x03u
#define GW_EVT_LOW_VOLTAGE      0x04u
#define GW_EVT_BOOT             0x05u
#define GW_EVT_WAIT_BYPASS      0x06u
#define GW_EVT_BOUND            0x07u
#define GW_EVT_REBIND           0x08u
#define GW_EVT_CALIB_DONE       0x09u
#define GW_EVT_CALIB_STEP       0x0Au
#define GW_EVT_CALIB_FAIL       0x0Bu
#define GW_EVT_REBIND_COMPLETE  0x0Cu
#define GW_EVT_NOT_EOL_TESTED   0x0Eu

/* Node fault flags */
#define GW_FAULT_LOCK        0x01u
#define GW_FAULT_VOLTAGE     0x02u
#define GW_FAULT_I2C         0x04u
#define GW_FAULT_OC          0x08u
#define GW_FAULT_ACTUATOR   0x10u
#define GW_FAULT_DISENGAGE  0x20u

/* CRC definitions */
#define GW_PACKET_CRC8_POLY 0x31u
#define GW_REBIND_CRC8_POLY 0x07u

#pragma pack(push, 1)

typedef struct
{
    uint8_t node;
    uint8_t type;
    uint8_t cmd;
    uint8_t value;
    uint8_t seq;
    uint8_t gwid;
    uint8_t crc8;
} GwLoRaCmd_t;

typedef struct
{
    uint8_t node;
    uint8_t type;
    uint8_t seq;
    uint8_t result;
    uint8_t gwid;
    uint8_t crc8;
} GwLoRaAck_t;

typedef struct
{
    uint8_t  node;
    uint8_t  type;
    uint8_t  seq;
    uint8_t  motor_state;
    uint8_t  fault_flags;
    uint16_t turns100;
    uint16_t voltage100;
    uint16_t current100;
    int8_t   rssi;
    uint8_t  gwid;
    uint8_t  crc8;
} GwLoRaStatus_t;

typedef struct
{
    uint8_t  node;
    uint8_t  type;
    uint8_t  seq;
    uint8_t  event;
    uint16_t voltage100;
    uint16_t current100;
    uint8_t  gwid;
    uint8_t  crc8;
} GwLoRaEvent_t;

typedef struct
{
    uint8_t node;
    uint8_t type;
    uint8_t cmd;
    uint8_t seq;
    uint8_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint8_t wday;
    uint8_t gwid;
    uint8_t crc8;
} GwLoRaSetTime_t;

typedef struct
{
    uint8_t node;
    uint8_t type;
    uint8_t cmd;
    uint8_t seq;
    uint8_t slot;
    uint8_t enabled;
    uint8_t action;
    uint8_t hour;
    uint8_t minute;
    uint8_t days;
    uint8_t gwid;
    uint8_t crc8;
} GwLoRaSetSchedule_t;

typedef struct
{
    uint8_t node;
    uint8_t type;
    uint8_t seq;
    uint8_t slot;
    uint8_t enabled;
    uint8_t action;
    uint8_t hour;
    uint8_t minute;
    uint8_t days;
    uint8_t gwid;
    uint8_t crc8;
} GwLoRaSchedStatus_t;

typedef struct
{
    uint8_t node;
    uint8_t type;
    uint8_t cmd;
    uint8_t new_gwid;
    uint8_t auth_token;
    uint8_t seq;
    uint8_t current_gwid;
    uint8_t crc8;
} GwLoRaRebindCmd_t;

typedef struct
{
    uint8_t node;
    uint8_t type;
    uint8_t seq;
    uint8_t event;
    uint8_t old_gwid;
    uint8_t new_gwid;
    uint8_t gwid;
    uint8_t crc8;
} GwLoRaRebindEvent_t;

#pragma pack(pop)

/* Wire-size assertions */
_Static_assert(sizeof(GwLoRaCmd_t) == 7, "GwLoRaCmd_t size");
_Static_assert(sizeof(GwLoRaAck_t) == 6, "GwLoRaAck_t size");
_Static_assert(sizeof(GwLoRaStatus_t) == 14, "GwLoRaStatus_t size");
_Static_assert(sizeof(GwLoRaEvent_t) == 10, "GwLoRaEvent_t size");
_Static_assert(sizeof(GwLoRaSetTime_t) == 13, "GwLoRaSetTime_t size");
_Static_assert(sizeof(GwLoRaSetSchedule_t) == 12,
               "GwLoRaSetSchedule_t size");
_Static_assert(sizeof(GwLoRaSchedStatus_t) == 11,
               "GwLoRaSchedStatus_t size");
_Static_assert(sizeof(GwLoRaRebindCmd_t) == 8, "GwLoRaRebindCmd_t size");
_Static_assert(sizeof(GwLoRaRebindEvent_t) == 8,
               "GwLoRaRebindEvent_t size");

/* CRC helpers */
uint8_t gwPacketCrc8(const uint8_t *data, size_t length);

uint8_t gwRebindAuthCrc8(uint8_t current_gwid,
                         uint8_t new_gwid,
                         uint8_t seq,
                         uint8_t rebind_secret);

bool gwPacketCrcValid(const uint8_t *packet, size_t length);

#ifdef __cplusplus
}
#endif

#endif /* GW_PROTOCOL_H */