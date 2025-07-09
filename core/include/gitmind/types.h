/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_TYPES_H
#define GITMIND_TYPES_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file types.h
 * @brief Core type definitions for git-mind
 */

/* Size constants */
#define GM_SHA1_SIZE 20
#define GM_SHA256_SIZE 32
#define GM_PATH_MAX 256
#define GM_ULID_SIZE 26
#define GM_FORMAT_BUFFER_SIZE 512

/* Time constants */
#define MILLIS_PER_SECOND 1000ULL
#define NANOS_PER_MILLI 1000000ULL

/* Relationship types */
typedef enum {
    GM_REL_IMPLEMENTS = 1,
    GM_REL_REFERENCES = 2,
    GM_REL_DEPENDS_ON = 3,
    GM_REL_AUGMENTS = 4,
    GM_REL_CUSTOM = 1000
} gm_rel_type_t;

/* Include sub-types */

#ifdef __cplusplus
}
#endif

#endif /* GITMIND_TYPES_H */
