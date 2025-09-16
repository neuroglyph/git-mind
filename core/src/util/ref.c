/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/util/ref.h"
#include "gitmind/error.h"
#include "gitmind/security/string.h"
#include "gitmind/constants_internal.h"

#include <git2.h>

int gm_build_ref(char *out, size_t out_sz, const char *prefix,
                 const char *branch) {
    if (!out || out_sz == 0 || !prefix || !branch) {
        return GM_ERR_INVALID_ARGUMENT;
    }
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
    if (rn < 0 || (size_t)rn >= sizeof candidate) {
        return GM_ERR_BUFFER_TOO_SMALL;
    }

    /* Validate ref name */
    int valid = 0;
    int vrc = git_reference_name_is_valid(&valid, candidate);
    if (vrc != 0 || valid == 0) {
        return GM_ERR_INVALID_ARGUMENT;
    }

    rn = gm_snprintf(out, out_sz, "%s", candidate);
    if (rn < 0 || (size_t)rn >= out_sz) {
        return GM_ERR_BUFFER_TOO_SMALL;
    }
    return GM_OK;
}
