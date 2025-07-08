/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
#ifndef GITMIND_TYPES_ULID_H
#define GITMIND_TYPES_ULID_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "gitmind/result.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ULID size constants */
#define GM_ULID_SIZE 26
#define GM_ULID_BUFFER_SIZE (GM_ULID_SIZE + 1)

/* Result type for ULID operations */
GM_RESULT_DEF(gm_result_ulid, char *);

/**
 * Generate a new ULID (Universally Unique Lexicographically Sortable ID).
 *
 * The ULID consists of:
 * - 48-bit timestamp (millisecond precision)
 * - 80-bit randomness
 *
 * Total of 128 bits encoded as 26 character base32 string.
 *
 * @param buffer Output buffer (must be at least GM_ULID_BUFFER_SIZE bytes)
 * @return Result containing pointer to buffer on success
 */
gm_result_ulid_t gm_ulid_generate(char *buffer);

/**
 * Generate a ULID with custom timestamp.
 *
 * @param buffer Output buffer (must be at least GM_ULID_BUFFER_SIZE bytes)
 * @param timestamp_ms Unix timestamp in milliseconds
 * @return Result containing pointer to buffer on success
 */
gm_result_ulid_t gm_ulid_generate_with_timestamp(char *buffer,
                                                  uint64_t timestamp_ms);

/**
 * Validate a ULID string.
 *
 * @param ulid String to validate
 * @return true if valid ULID, false otherwise
 */
bool gm_ulid_is_valid(const char *ulid);

/**
 * Extract timestamp from ULID.
 *
 * @param ulid ULID string
 * @param timestamp_ms Output timestamp in milliseconds
 * @return Result containing success status
 */
gm_result_void_t gm_ulid_get_timestamp(const char *ulid,
                                        uint64_t *timestamp_ms);

/**
 * Compare two ULIDs.
 *
 * @param ulid1 First ULID
 * @param ulid2 Second ULID
 * @return <0 if ulid1 < ulid2, 0 if equal, >0 if ulid1 > ulid2
 */
int gm_ulid_compare(const char *ulid1, const char *ulid2);

#ifdef __cplusplus
}
#endif

#endif /* GITMIND_TYPES_ULID_H */
