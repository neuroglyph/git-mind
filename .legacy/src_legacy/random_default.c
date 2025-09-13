/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L

#include "gitmind/random_ops.h"

#include <stdlib.h>

/* Default random operations - use real system calls */
static const gm_random_ops_t default_random_ops = {
    .rand = rand, .srand = srand, .rand_r = rand_r};

const gm_random_ops_t *gm_random_ops_default(void) {
    return &default_random_ops;
}