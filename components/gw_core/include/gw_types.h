#ifndef GW_TYPES_H
#define GW_TYPES_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Production Gateway runtime operational state.
 *
 * EOL/manufacturing operation is intentionally excluded from
 * the production Gateway firmware state model.
 */
typedef enum
{
	GW_STATE_UNBOUND  = 0,
	GW_STATE_BOUND    = 1,
	GW_STATE_DEGRADED = 2,
	GW_STATE_FAULT    = 3
} GwRuntimeState_t;

/**
 * Generic result used by Gateway core modules.
 */
typedef enum
{
	GW_RESULT_OK = 0,
	GW_RESULT_INVALID_ARG,
	GW_RESULT_NOT_INITIALIZED,
	GW_RESULT_ALREADY_INITIALIZED,
	GW_RESULT_BUSY,
	GW_RESULT_NOT_READY,
	GW_RESULT_TIMEOUT,
	GW_RESULT_ERROR
} GwResult_t;

#ifdef __cplusplus
}
#endif

#endif /* GW_TYPES_H */
