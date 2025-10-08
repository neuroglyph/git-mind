/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_CACHE_INTERNAL_STALENESS_H
#define GITMIND_CACHE_INTERNAL_STALENESS_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Decide if cache is stale based on journal tip time and a max-age window. */
static inline bool gm_cache_staleness_time(uint64_t journal_tip_time,
                                           uint64_t now_time,
                                           uint64_t max_age_seconds) {
    if (now_time <= journal_tip_time) return false;
    return (now_time - journal_tip_time) > max_age_seconds;
}

#ifdef __cplusplus
}
#endif

#endif /* GITMIND_CACHE_INTERNAL_STALENESS_H */

