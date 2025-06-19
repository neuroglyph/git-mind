/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_CONSTANTS_H
#define GITMIND_CONSTANTS_H

/* String constants to avoid typos */
#define GM_STR_IMPLEMENTS    "implements"
#define GM_STR_REFERENCES    "references"
#define GM_STR_DEPENDS_ON    "depends_on"
#define GM_STR_DEPENDS_DASH  "depends-on"
#define GM_STR_AUGMENTS      "augments"
#define GM_STR_CUSTOM        "custom"

/* Environment variable names */
#define GM_ENV_SOURCE        "GIT_MIND_SOURCE"
#define GM_ENV_AUTHOR        "GIT_MIND_AUTHOR"
#define GM_ENV_SESSION       "GIT_MIND_SESSION"
#define GM_ENV_BRANCH        "GIT_MIND_BRANCH"
#define GM_ENV_NO_CACHE      "GIT_MIND_NO_CACHE"
#define GM_ENV_VERBOSE       "GIT_MIND_VERBOSE"

/* Environment variable values for source types */
#define GM_ENV_VAL_HUMAN     "human"
#define GM_ENV_VAL_CLAUDE    "claude"
#define GM_ENV_VAL_GPT       "gpt"
#define GM_ENV_VAL_SYSTEM    "system"
#define GM_ENV_VAL_IMPORT    "import"

/* CLI flag strings */
#define GM_FLAG_TYPE         "--type"
#define GM_FLAG_CONFIDENCE   "--confidence"
#define GM_FLAG_SOURCE       "--source"
#define GM_FLAG_MIN_CONF     "--min-confidence"
#define GM_FLAG_LANE         "--lane"
#define GM_FLAG_PENDING      "--pending"
#define GM_FLAG_SHOW_ATTR    "--show-attribution"
#define GM_FLAG_FROM         "--from"
#define GM_FLAG_VERBOSE      "--verbose"
#define GM_FLAG_HELP         "--help"

/* Default values */
#define GM_DEFAULT_REL_TYPE  GM_STR_REFERENCES
#define GM_DEFAULT_CONFIDENCE 0x3C00  /* 1.0 in half-float */
#define GM_AI_DEFAULT_CONFIDENCE 0x3666  /* 0.85 in half-float */

/* Output format constants */
#define GM_SUCCESS_CREATED   "Created link: %s"
#define GM_SUCCESS_FILTERED  "Showing %zu edges (filtered by %s)"
#define GM_SUCCESS_TOTAL     "Total edges: %zu"

/* Error message constants */
#define GM_ERR_USAGE_LINK    "Usage: git-mind link <source> <target> [--type <type>] [--confidence <float>]"
#define GM_ERR_TYPES_HELP    "Types: implements, references, depends_on, augments"
#define GM_ERR_WRITE_FAILED  "Error: Failed to write link"
#define GM_ERR_INVALID_CONF  "Error: Invalid confidence value (must be 0.0-1.0)"
#define GM_ERR_PARSE_ARGS    "Error: Failed to parse arguments"

/* Format buffer sizes */
#define GM_FORMAT_BUFFER_SIZE  512
#define GM_LINE_BUFFER_SIZE    1024

/* Confidence conversion constants */
#define GM_CONFIDENCE_SCALE    0x3C00  /* 1.0 in IEEE 754 half-float */
#define GM_CONFIDENCE_MIN      0.0f
#define GM_CONFIDENCE_MAX      1.0f

#endif /* GITMIND_CONSTANTS_H */