/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/attribution.h"

#include "gitmind/attribution/internal/defaults.h"
#include "gitmind/attribution/internal/env_loader.h"
#include "gitmind/error.h"
#include "gitmind/ports/env_port.h"

static int finish_result(gm_result_void_t result) {
    if (result.ok) {
        return 0;
    }

    if (result.u.err != NULL) {
        gm_error_free(result.u.err);
    }
    return -1;
}

int gm_attribution_set_default(gm_attribution_t *attr, gm_source_type_t source) {
    return finish_result(gm_attribution_defaults_apply(attr, source));
}

int gm_attribution_from_env(gm_attribution_t *attr) {
    const gm_env_port_t *port = gm_env_port_system();
    return finish_result(gm_attribution_from_env_with_port(attr, port));
}
