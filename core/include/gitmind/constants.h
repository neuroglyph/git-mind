/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_CONSTANTS_H
#define GITMIND_CONSTANTS_H

/**
 * @file constants.h
 * @brief String and numeric constants for git-mind
 */

/* String constants for relationship types */
#define GM_STR_IMPLEMENTS "implements"
#define GM_STR_REFERENCES "references"
#define GM_STR_DEPENDS_ON "depends_on"
#define GM_STR_AUGMENTS "augments"
#define GM_STR_CUSTOM "custom"

/* Environment variable values for source types */
#define GM_ENV_VAL_HUMAN "human"
#define GM_ENV_VAL_CLAUDE "claude"
#define GM_ENV_VAL_GPT "gpt"
#define GM_ENV_VAL_SYSTEM "system"

/* Confidence range constants */
#define GM_CONFIDENCE_MIN 0.0f
#define GM_CONFIDENCE_MAX 1.0f

/* Format buffer sizes */
#define GM_FORMAT_BUFFER_SIZE 512

/* Cache system constants */
#define BITS_PER_HEX_CHAR 4
#define HEX_CHARS_PER_BYTE 2
#define SHA_HEX_SIZE (GM_OID_RAWSZ * HEX_CHARS_PER_BYTE + 1)
#define ZERO_SHA_STRING "0000000000000000000000000000000000000000"
#define EDGE_MAP_BUCKETS 1024
#define DIR_PERMS_NORMAL 0755
#define REF_NAME_BUFFER_SIZE 256
#define RM_COMMAND_EXTRA_SIZE 10
#define CACHE_SIZE_ESTIMATE_PER_EDGE 32

/* Legacy error constants (for compatibility) */
#define GM_NOT_FOUND GM_ERR_NOT_FOUND
#define GM_NO_MEMORY GM_ERR_OUT_OF_MEMORY 
#define GM_IO_ERROR GM_ERR_IO_FAILED
#define GM_INVALID_ARG GM_ERR_INVALID_ARGUMENT

#endif /* GITMIND_CONSTANTS_H */
