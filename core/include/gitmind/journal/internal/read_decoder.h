/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_JOURNAL_INTERNAL_READ_DECODER_H
#define GITMIND_JOURNAL_INTERNAL_READ_DECODER_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "gitmind/edge.h"
#include "gitmind/edge_attributed.h"
#include "gitmind/result.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Decode a single edge from CBOR bytes.
 * - If prefer_attributed is true, try attributed first and fall back to legacy.
 * - If false, try legacy first and fall back to attributed; when falling back,
 *   the attributed edge is converted into a basic edge.
 * Output parameters are optional and may be NULL when the caller does not need
 * a particular result:
 *   - out_basic: populated on success when the result is a basic edge.
 *   - out_attr: populated on success when the result is an attributed edge.
 *   - consumed: set to bytes read on success; left untouched on failure.
 *   - got_attr: set to true when an attributed edge is produced, false when a
 *     basic edge is produced; left untouched on failure.
 * Invariants on success:
 *   - Exactly one of (out_basic, out_attr) is written based on got_attr.
 *   - consumed <= len when provided.
 */
GM_NODISCARD gm_result_void_t gm_journal_decode_edge(const uint8_t *buf,
                                                     size_t len,
                                                     bool prefer_attributed,
                                                     gm_edge_t *out_basic,
                                                     gm_edge_attributed_t *out_attr,
                                                     size_t *consumed,
                                                     bool *got_attr);

#ifdef __cplusplus
}
#endif

#endif /* GITMIND_JOURNAL_INTERNAL_READ_DECODER_H */
