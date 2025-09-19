/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/util/ref.h"
#include "gitmind/error.h"
#include "gitmind/security/string.h"
#include "gitmind/constants_internal.h"

#include <stdbool.h>
#include <string.h>

static bool branch_has_forbidden_prefix(const char *branch) {
    return branch[0] == 'r' && branch[1] == 'e' && branch[2] == 'f' &&
           branch[3] == 's' && branch[4] == '/';
}

static bool branch_has_edge_slashes(const char *branch) {
    size_t length = strlen(branch);
    return length == 0 || branch[0] == '/' || branch[length - 1] == '/';
}

static bool branch_contains_invalid_char(const char *branch) {
    for (const char *cursor = branch; *cursor; ++cursor) {
        switch (*cursor) {
        case '~':
        case '^':
        case ':':
        case '?':
        case '[':
        case '*':
        case '\\':
            return true;
        default:
            break;
        }
    }
    return false;
}

static bool branch_has_forbidden_sequence(const char *branch) {
    return strstr(branch, "..") != NULL || strstr(branch, "@{") != NULL;
}

static int validate_branch(const char *branch) {
    if (*branch == '\0') {
        return GM_ERR_INVALID_ARGUMENT;
    }
    if (branch_has_forbidden_prefix(branch)) {
        return GM_ERR_INVALID_ARGUMENT;
    }
    if (branch_has_edge_slashes(branch)) {
        return GM_ERR_INVALID_ARGUMENT;
    }
    if (branch_contains_invalid_char(branch)) {
        return GM_ERR_INVALID_ARGUMENT;
    }
    if (branch_has_forbidden_sequence(branch)) {
        return GM_ERR_INVALID_ARGUMENT;
    }
    return GM_OK;
}

int gm_build_ref(char *out, size_t out_sz, const char *prefix,
                 const char *branch) {
    if (!out || out_sz == 0 || !prefix || !branch) {
        if (out && out_sz > 0) {
            out[0] = '\0';
        }
        return GM_ERR_INVALID_ARGUMENT;
    }
    out[0] = '\0';
    /* Require shorthand branch (no leading "refs/") to avoid double prefixing */
    {
        int validation_rc = validate_branch(branch);
        if (validation_rc != GM_OK) {
            return validation_rc;
        }
    }

    char candidate[REF_NAME_BUFFER_SIZE];
    int written =
        gm_snprintf(candidate, sizeof candidate, "%s%s", prefix, branch);
    if (written < 0) {
        return GM_ERR_UNKNOWN;
    }
    if ((size_t)written >= sizeof candidate) {
        return GM_ERR_BUFFER_TOO_SMALL;
    }

    written = gm_snprintf(out, out_sz, "%s", candidate);
    if (written < 0) {
        out[0] = '\0';
        return GM_ERR_UNKNOWN;
    }
    if ((size_t)written >= out_sz) {
        out[0] = '\0';
        return GM_ERR_BUFFER_TOO_SMALL;
    }
    return GM_OK;
}
