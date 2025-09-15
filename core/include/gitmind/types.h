/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_TYPES_H
#define GITMIND_TYPES_H

#include <stddef.h>
#include <stdint.h>
/* Pull in libgit2 OID for public gm_oid_t definition */
#include <git2/oid.h>

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
#ifndef GIT_OID_RAWSZ
#define GIT_OID_RAWSZ 20
#endif
#define GM_OID_RAWSZ GIT_OID_RAWSZ
#ifndef GIT_OID_HEXSZ
#define GM_OID_HEX_CHARS 40
#else
#define GM_OID_HEX_CHARS GIT_OID_HEXSZ
#endif
#define GM_PATH_MAX 4096
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
/**
 * OID type (SHA-agnostic). Use instead of raw byte arrays in new code.
 */
typedef git_oid gm_oid_t;

#ifdef __cplusplus
}
#endif

#endif /* GITMIND_TYPES_H */
