/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_CONSTANTS_H
#define GITMIND_CONSTANTS_H

/* String constants to avoid typos */
#define GM_STR_IMPLEMENTS "implements"
#define GM_STR_REFERENCES "references"
#define GM_STR_DEPENDS_ON "depends_on"
#define GM_STR_DEPENDS_DASH "depends-on"
#define GM_STR_AUGMENTS "augments"
#define GM_STR_CUSTOM "custom"

/* Environment variable names */
#define GM_ENV_SOURCE "GIT_MIND_SOURCE"
#define GM_ENV_AUTHOR "GIT_MIND_AUTHOR"
#define GM_ENV_SESSION "GIT_MIND_SESSION"
#define GM_ENV_BRANCH "GIT_MIND_BRANCH"
#define GM_ENV_NO_CACHE "GIT_MIND_NO_CACHE"
#define GM_ENV_VERBOSE "GIT_MIND_VERBOSE"

/* Environment variable values for source types */
#define GM_ENV_VAL_HUMAN "human"
#define GM_ENV_VAL_CLAUDE "claude"
#define GM_ENV_VAL_GPT "gpt"
#define GM_ENV_VAL_SYSTEM "system"
#define GM_ENV_VAL_IMPORT "import"

/* CLI flag strings */
#define GM_FLAG_TYPE "--type"
#define GM_FLAG_CONFIDENCE "--confidence"
#define GM_FLAG_SOURCE "--source"
#define GM_FLAG_MIN_CONF "--min-confidence"
#define GM_FLAG_LANE "--lane"
#define GM_FLAG_PENDING "--pending"
#define GM_FLAG_SHOW_ATTR "--show-attribution"
#define GM_FLAG_FROM "--from"
#define GM_FLAG_VERBOSE "--verbose"
#define GM_FLAG_HELP "--help"
#define GM_FLAG_SHOW_AUG "--show-augments"
#define GM_FLAG_BRANCH "--branch"
#define GM_FLAG_FORCE "--force"

/* Default values */
#define GM_DEFAULT_REL_TYPE GM_STR_REFERENCES
#define GM_DEFAULT_CONFIDENCE 0x3C00    /* 1.0 in half-float */
#define GM_AI_DEFAULT_CONFIDENCE 0x3666 /* 0.85 in half-float */

/* Output format constants */
#define GM_SUCCESS_CREATED "Created link: %s"
#define GM_SUCCESS_FILTERED "Showing %zu edges (filtered by %s)"
#define GM_SUCCESS_TOTAL "Total edges: %zu"

/* Error message constants */
#define GM_ERR_USAGE_LINK                                                      \
    "Usage: git-mind link <source> <target> [--type <type>] [--confidence "    \
    "<float>]"
#define GM_ERR_TYPES_HELP "Types: implements, references, depends_on, augments"
#define GM_ERR_WRITE_FAILED "Error: Failed to write link"
#define GM_ERR_INVALID_CONF "Error: Invalid confidence value (must be 0.0-1.0)"
#define GM_ERR_PARSE_ARGS "Error: Failed to parse arguments"

/* Format buffer sizes */
#define GM_FORMAT_BUFFER_SIZE 512
#define GM_LINE_BUFFER_SIZE 1024

/* Confidence conversion constants */
#define GM_CONFIDENCE_SCALE 0x3C00 /* 1.0 in IEEE 754 half-float */
#define GM_CONFIDENCE_MIN 0.0f
#define GM_CONFIDENCE_MAX 1.0f

/* Callback return codes */
#define GM_CALLBACK_CONTINUE 0
#define GM_CALLBACK_STOP 1

/* Source filter values */
#define GM_FILTER_VAL_AI "ai"
#define GM_FILTER_VAL_ALL "all"

/* List output messages */
#define GM_MSG_NO_LINKS "No links found"
#define GM_MSG_NO_LINKS_PATH "No links found for: %s"
#define GM_MSG_NO_LINKS_FILTER "No links found matching filter criteria"

/* Cache-related constants */
#define GM_ERROR_UNKNOWN_OPT "Error: Unknown option '%s'"
#define GM_ERROR_GET_BRANCH "Error: Failed to get current branch"
#define GM_ERROR_CACHE_FAILED "Error: Cache rebuild failed: %s"
#define GM_MSG_CACHE_CURRENT "Cache is up to date for branch '%s'"
#define GM_MSG_CACHE_REBUILD "Rebuilding cache for branch '%s'..."
#define GM_MSG_CACHE_SUCCESS "Cache rebuilt successfully!"
#define GM_MSG_CACHE_STATS                                                     \
    "  Edges indexed: %llu\n  Cache size: ~%llu KB\n  Build time: %.2f "       \
    "seconds"
#define GM_MSG_CACHE_PERF                                                      \
    "\nQueries will now use the bitmap cache for O(log N) performance."

/* Unit conversion */
#define GM_BYTES_PER_KB 1024

/* Install hooks constants */
#define GM_HOOK_BACKUP_SUFFIX ".backup"
#define GM_MSG_HOOK_EXISTS "Existing post-commit hook found"
#define GM_MSG_HOOK_BACKUP "Backing up to: %s"
#define GM_MSG_HOOK_INSTALLED "git-mind hooks installed successfully"
#define GM_MSG_HOOK_ALREADY "git-mind hooks already installed"
#define GM_MSG_HOOK_DETAILS                                                    \
    "\nThe post-commit hook will automatically create AUGMENTS edges\nwhen "   \
    "you modify files that have existing semantic links.\n\nTo test: modify "  \
    "a linked file and commit the change."
#define GM_ERR_HOOK_BACKUP "Failed to backup existing hook: %s"
#define GM_ERR_HOOK_CREATE "Failed to create post-commit hook: %s"
#define GM_ERR_HOOK_WRITE "Failed to write hook script"
#define GM_ERR_HOOK_CHMOD "Failed to make hook executable: %s"
#define GM_ERR_HOOK_NO_DIR                                                     \
    "Error: .git/hooks directory not found\nAre you in a git repository?"

/* Single edge operation */
#define GM_SINGLE_EDGE_COUNT 1

/* Option prefix check */
#define GM_OPTION_PREFIX '-'

/* Hook installation constants */
#define GM_HOOK_PATH ".git/hooks/post-commit"
#define GM_HOOKS_DIR ".git/hooks"
#define GM_HOOK_IDENTIFIER "git-mind post-commit hook"
#define GM_HOOK_PERMS 0755

/* Buffer sizes */
#define GM_HOOK_BUFFER_SIZE 256

/* Filter descriptions */
#define GM_FILTER_DESC_CONF "confidence filter"

/* Error messages for list command */
#define GM_ERR_READ_LINKS "Error: Failed to read links"

/* Generic error format */
#define GM_ERR_FORMAT "Error: %s"

/* Format strings */
#define GM_FMT_CONFIDENCE "%.3f"
#define GM_FMT_TIME_SECONDS "%.2f"

#endif /* GITMIND_CONSTANTS_H */