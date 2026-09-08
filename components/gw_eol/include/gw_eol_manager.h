#ifndef GW_EOL_MANAGER_H
#define GW_EOL_MANAGER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "gw_eol_protocol.h"
#include "gw_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------- */
/* Gateway EOL manager lifecycle                                              */
/* -------------------------------------------------------------------------- */

GwResult_t gwEolManagerInit(void);

GwResult_t gwEolManagerDeinit(void);

bool gwEolManagerIsInitialized(void);

/* -------------------------------------------------------------------------- */
/* EOL session control                                                        */
/* -------------------------------------------------------------------------- */

GwResult_t gwEolManagerStart(
    uint8_t node,
    uint8_t gwid);

GwResult_t gwEolManagerCancel(void);

/* -------------------------------------------------------------------------- */
/* Individual test control                                                    */
/* -------------------------------------------------------------------------- */

GwResult_t gwEolManagerRequestTest(
    uint8_t test_idx);

/* -------------------------------------------------------------------------- */
/* Incoming EOL packet processing                                             */
/* -------------------------------------------------------------------------- */

GwResult_t gwEolManagerProcessRx(
    const uint8_t *data,
    size_t length);

/* -------------------------------------------------------------------------- */
/* EOL session state                                                          */
/* -------------------------------------------------------------------------- */

bool gwEolManagerIsActive(void);

uint8_t gwEolManagerGetNode(void);

uint8_t gwEolManagerGetSequence(void);

uint8_t gwEolManagerGetPassCount(void);

uint8_t gwEolManagerGetFailCount(void);

#ifdef __cplusplus
}
#endif

#endif /* GW_EOL_MANAGER_H */
