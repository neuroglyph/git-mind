/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/util/errno_compat.h"

#include <errno.h>
#include <string.h>

#include "gitmind/error.h"
#include "gitmind/result.h"

gm_result_void_t gm_errno_to_result(const char *operation, const char *path,
                                    int err) {
    if (operation == NULL) {
        operation = "operation";
    }
    if (path == NULL) {
        path = "<unknown>";
    }

    int32_t code = GM_ERR_IO_FAILED;
    switch (err) {
    case ENOENT:
        code = GM_ERR_NOT_FOUND;
        break;
    case EACCES:
        code = GM_ERR_PERMISSION_DENIED;
        break;
    case ENAMETOOLONG:
        code = GM_ERR_PATH_TOO_LONG;
        break;
    case EROFS:
        code = GM_ERR_READ_ONLY;
        break;
    case EEXIST:
        code = GM_ERR_ALREADY_EXISTS;
        break;
    default:
        code = GM_ERR_IO_FAILED;
        break;
    }

    return gm_err_void(
        GM_ERROR(code, "%s failed for %s: %s", operation, path, strerror(err)));
}
