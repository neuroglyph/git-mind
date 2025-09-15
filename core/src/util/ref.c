/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/util/ref.h"
#include "gitmind/error.h"
#include "gitmind/security/string.h"

static int branch_valid(const char *b) {
    if (!b || !*b) return 0;
    for (const char *p = b; *p; ++p) {
        if (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') return 0;
        if (*p == '/') return 0; /* tree path not allowed here */
    }
    if (b[0] == '.' && b[1] == '.' ) return 0; /* no .. */
    return 1;
}

int gm_build_ref(char *out, size_t out_sz, const char *prefix,
                 const char *branch) {
    if (!out || out_sz == 0 || !prefix || !branch) {
        return GM_ERR_INVALID_ARGUMENT;
    }
    if (!branch_valid(branch)) {
        return GM_ERR_INVALID_ARGUMENT;
    }
    int rn = gm_snprintf(out, out_sz, "%s%s", prefix, branch);
    if (rn < 0 || (size_t)rn >= out_sz) {
        return GM_ERR_BUFFER_TOO_SMALL;
    }
    return GM_OK;
}

