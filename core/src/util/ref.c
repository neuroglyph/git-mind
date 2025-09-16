/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/util/ref.h"
#include "gitmind/error.h"
#include "gitmind/security/string.h"
#include "gitmind/constants_internal.h"
#include <string.h>

#include <git2.h>

int gm_build_ref(char *out, size_t out_sz, const char *prefix,
                 const char *branch) {
    if (!out || out_sz == 0 || !prefix || !branch) {
        if (out && out_sz > 0) out[0] = '\0';
        return GM_ERR_INVALID_ARGUMENT;
    }
    out[0] = '\0';
    if (*branch == '\0') {
        return GM_ERR_INVALID_ARGUMENT;
    }
    /* Require shorthand branch (no leading "refs/") to avoid double prefixing */
    if (branch[0] == 'r' && branch[1] == 'e' && branch[2] == 'f' &&
        branch[3] == 's' && branch[4] == '/') {
        return GM_ERR_INVALID_ARGUMENT;
    }

    char candidate[REF_NAME_BUFFER_SIZE];
    int rn = gm_snprintf(candidate, sizeof candidate, "%s%s", prefix, branch);
    if (rn < 0) {
        return GM_ERR_UNKNOWN;
    }
    if ((size_t)rn >= sizeof candidate) {
        return GM_ERR_BUFFER_TOO_SMALL;
    }

    /* Minimal validity checks without libgit2 dependency */
    {
        const char *p = branch;
        if (*p == '/' || p[strlen(p) - 1] == '/') {
            return GM_ERR_INVALID_ARGUMENT;
        }
        for (; *p; ++p) {
            const char c = *p;
            if (c == '~' || c == '^' || c == ':' || c == '?' ||
                c == '[' || c == '*' || c == '\\') {
                return GM_ERR_INVALID_ARGUMENT;
            }
        }
        if (strstr(branch, "..") != NULL) return GM_ERR_INVALID_ARGUMENT;
        if (strstr(branch, "@{") != NULL) return GM_ERR_INVALID_ARGUMENT;
    }

    rn = gm_snprintf(out, out_sz, "%s", candidate);
    if (rn < 0) {
        out[0] = '\0';
        return GM_ERR_UNKNOWN;
    }
    if ((size_t)rn >= out_sz) {
        out[0] = '\0';
        return GM_ERR_BUFFER_TOO_SMALL;
    }
    return GM_OK;
}
