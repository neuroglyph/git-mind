/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_RANDOM_OPS_H
#define GITMIND_RANDOM_OPS_H

#include <stdlib.h>

/**
 * Random number operations interface for dependency injection.
 * This allows for test doubles and deterministic testing.
 */
typedef struct gm_random_ops {
    int (*rand)(void);
    void (*srand)(unsigned int seed);
    int (*rand_r)(unsigned int *seedp);
} gm_random_ops_t;

/**
 * Get default random operations (uses real system calls).
 */
const gm_random_ops_t *gm_random_ops_default(void);

#endif /* GITMIND_RANDOM_OPS_H */