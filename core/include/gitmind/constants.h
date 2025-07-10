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

#endif /* GITMIND_CONSTANTS_H */
