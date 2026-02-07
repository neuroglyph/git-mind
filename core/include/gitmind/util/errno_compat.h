/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_UTIL_ERRNO_COMPAT_H
#define GITMIND_UTIL_ERRNO_COMPAT_H

#include <stdint.h>

#include "gitmind/result.h"

#ifdef __cplusplus
extern "C" {
#endif

gm_result_void_t gm_errno_to_result(const char *operation, const char *path,
                                    int err);

#ifdef __cplusplus
}
#endif

#endif /* GITMIND_UTIL_ERRNO_COMPAT_H */
