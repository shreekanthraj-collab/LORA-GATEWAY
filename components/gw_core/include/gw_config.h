#ifndef GW_CONFIG_H
#define GW_CONFIG_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Production Gateway configuration.
 *
 * Hardware-specific values must be supplied by the appropriate
 * platform/driver layer rather than being scattered through
 * application code.
 */

/* Gateway identity */
#define GW_CONFIG_GWID_UNBOUND 0x00u

/* Gateway limits */
#define GW_CONFIG_MAX_NODES 256u

#ifdef __cplusplus
}
#endif

#endif /* GW_CONFIG_H */
