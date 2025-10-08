/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_TELEMETRY_INTERNAL_LOG_FORMAT_H
#define GITMIND_TELEMETRY_INTERNAL_LOG_FORMAT_H

#include <stdbool.h>
#include <stddef.h>

#include "gitmind/result.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct gm_log_kv {
    const char *key;
    const char *value;
} gm_log_kv_t;

/*
 * Formatter function type. Implementations must write a single line into `out`.
 * When `json` is true, render a compact JSON object; otherwise render text/logfmt.
 */
typedef gm_result_void_t (*gm_log_formatter_fn)(const gm_log_kv_t *kvs,
                                                size_t kv_count,
                                                bool json,
                                                char *out,
                                                size_t out_size);

/* Default renderer: JSON or text (event=<name> k=v). */
gm_result_void_t gm_log_format_render_default(const gm_log_kv_t *kvs,
                                              size_t kv_count,
                                              bool json,
                                              char *out,
                                              size_t out_size);

#ifdef __cplusplus
}
#endif

#endif /* GITMIND_TELEMETRY_INTERNAL_LOG_FORMAT_H */

