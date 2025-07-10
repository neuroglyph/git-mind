/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_CONSTANTS_INTERNAL_H
#define GITMIND_CONSTANTS_INTERNAL_H

/* Buffer Sizes */
#define BUFFER_SIZE_TINY 256    /* Small buffers (e.g., git config lines) */
#define BUFFER_SIZE_SMALL 1024  /* Standard small buffer (e.g., CWD) */
#define BUFFER_SIZE_MEDIUM 4096 /* Medium buffer (paths, commands) */
#define BUFFER_SIZE_LARGE 65536 /* Large buffer (file content) */

/* Numeric Base Constants */
#define BITS_PER_HEX_CHAR 4  /* Bits per hexadecimal character */
#define HEX_CHARS_PER_BYTE 2 /* Hex characters per byte */

/* Path and String Sizes */
#define PATH_BUFFER_SIZE 4096 /* Maximum path length */
#define REF_NAME_BUFFER_SIZE                                                   \
    512 /* Git reference name buffer (must fit prefix + branch) */
#define SHA_HEX_SIZE 41   /* SHA1 hex string size (40 + null) */
#define SHA_BYTES_SIZE 20 /* SHA1 size in bytes */
#define ZERO_SHA_STRING "0000000000000000000000000000000000000000"
#define RM_COMMAND_EXTRA_SIZE 32 /* Extra space for rm -rf command */

/* Time Conversion Constants */
#define MILLIS_PER_SECOND 1000
#define MICROS_PER_SECOND 1000000
#define NANOS_PER_SECOND 1000000000
#define NANOS_PER_MILLI 1000000
#define NANOS_PER_MICRO 1000

/* Git Constants */
#define EMPTY_TREE_SHA "4b825dc642cb6eb9a060e54bf8d69288fbee4904"
#define REFS_PREFIX "refs/"
#define REFS_HEADS_PREFIX "refs/heads/"
#define REFS_TAGS_PREFIX "refs/tags/"
#define HEAD_REF "HEAD"
#define GITMIND_NAMESPACE "refs/gitmind/"
#define GITMIND_JOURNAL_REF "refs/gitmind/journal"
#define GITMIND_CACHE_REF "refs/gitmind/cache"

/* Exit Codes */
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1
#define EXIT_USAGE_ERROR 2
#define EXIT_NOT_GIT_REPO 3
#define EXIT_PERMISSION_DENIED 4
#define EXIT_FILE_NOT_FOUND 5
#define EXIT_SAFETY_VIOLATION 42 /* Special exit code for safety checks */

/* File Permissions */
#define FILE_PERMS_READABLE 0644 /* -rw-r--r-- */
#define FILE_PERMS_PRIVATE 0600  /* -rw------- */
#define DIR_PERMS_NORMAL 0755    /* drwxr-xr-x */
#define DIR_PERMS_PRIVATE 0700   /* drwx------ */
#define HOOK_PERMS 0755          /* -rwxr-xr-x */

/* Safety Check Patterns */
#define SAFETY_PATTERN_GITMIND ".gitmind"
#define SAFETY_PATTERN_REFS ".git/refs"
#define SAFETY_PATTERN_OBJECTS ".git/objects"
#define SAFETY_PATTERN_GITDIR ".git/"

/* Dangerous Remote Patterns */
#define REMOTE_PATTERN_GITMIND "git-mind"
#define REMOTE_PATTERN_NEUROGLYPH "neuroglyph/git-mind"
#define REMOTE_PATTERN_GITMIND_GIT "git-mind.git"

/* Hook Script Constants */
#define PRE_PUSH_HOOK_NAME "pre-push"
#define POST_COMMIT_HOOK_NAME "post-commit"
#define HOOK_BACKUP_SUFFIX ".gitmind-backup"
#define HOOK_SHEBANG "#!/bin/sh\n"

/* Cache Build Constants */
#define CACHE_BUILD_BATCH_SIZE 1000    /* Edges per batch */
#define EDGE_MAP_BUCKETS 1024          /* Hash map bucket count */
#define KB_SIZE 1024                   /* Bytes per KB for display */
#define CACHE_SIZE_ESTIMATE_PER_EDGE 4 /* Estimated bytes per edge in cache */

/* Display Constants */
#define PROGRESS_UPDATE_INTERVAL 100 /* Update progress every N items */
#define SPINNER_CHARS "|/-\\"
#define SPINNER_COUNT 4

/* ULID Constants */
#define ULID_ENCODING "0123456789ABCDEFGHJKMNPQRSTVWXYZ"
#define ULID_ENCODING_MASK 0x1F /* 5 bits for base32 */
#define ULID_TIME_LENGTH 10
#define ULID_RANDOM_LENGTH 16
#define ULID_TOTAL_LENGTH 26

/* Limits */
#define MAX_COMMAND_LENGTH 32    /* Maximum CLI command name */
#define MAX_EDGE_TYPE_LENGTH 64  /* Maximum edge type string */
#define MAX_ERROR_MSG_LENGTH 512 /* Maximum error message */

/* Porcelain Output Keys */
#define PORCELAIN_KEY_STATUS "status"
#define PORCELAIN_KEY_BRANCH "branch"
#define PORCELAIN_KEY_EDGES "edges"
#define PORCELAIN_KEY_CACHE_SIZE_KB "cache_size_kb"
#define PORCELAIN_KEY_BUILD_TIME "build_time_seconds"
#define PORCELAIN_KEY_SOURCE "source"
#define PORCELAIN_KEY_TARGET "target"
#define PORCELAIN_KEY_TYPE "type"
#define PORCELAIN_KEY_FILE "file"
#define PORCELAIN_KEY_COUNT "count"
#define PORCELAIN_KEY_SHA "sha"
#define PORCELAIN_KEY_ACTION "action"
#define PORCELAIN_KEY_REASON "reason"
#define PORCELAIN_KEY_HOOK "hook"
#define PORCELAIN_KEY_CONFIDENCE "confidence"
#define PORCELAIN_KEY_ULID "ulid"

/* Porcelain Status Values */
#define PORCELAIN_STATUS_SUCCESS "success"
#define PORCELAIN_STATUS_UP_TO_DATE "up-to-date"
#define PORCELAIN_STATUS_ALREADY_INSTALLED "already-installed"
#define PORCELAIN_STATUS_CREATED "created"
#define PORCELAIN_STATUS_BACKED_UP "backed-up"
#define PORCELAIN_STATUS_INSTALLED "installed"
#define PORCELAIN_STATUS_FAILED "failed"

/* Porcelain Action Values */
#define PORCELAIN_ACTION_BACKUP "backup"
#define PORCELAIN_ACTION_INSTALL "install"
#define PORCELAIN_ACTION_LINK "link"

#endif /* GITMIND_CONSTANTS_INTERNAL_H */
