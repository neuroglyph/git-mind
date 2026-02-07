/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_DOMAIN_EDGE_ATTRIBUTED_CBOR_H
#define GITMIND_DOMAIN_EDGE_ATTRIBUTED_CBOR_H

#include <stddef.h>
#include <stdint.h>

#include "gitmind/result.h"

#ifdef __cplusplus
extern "C" {
#endif

struct gm_edge_attributed;

GM_NODISCARD gm_result_void_t gm_edge_attributed_cbor_encode(
    const struct gm_edge_attributed *edge,
    uint8_t *buffer,
    size_t *len);

GM_NODISCARD gm_result_void_t gm_edge_attributed_cbor_decode(
    const uint8_t *buffer,
    size_t len,
    struct gm_edge_attributed *edge_out,
    size_t *consumed);

#ifdef __cplusplus
}
#endif

#endif /* GITMIND_DOMAIN_EDGE_ATTRIBUTED_CBOR_H */
