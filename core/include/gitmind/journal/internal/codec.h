/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_JOURNAL_INTERNAL_CODEC_H
#define GITMIND_JOURNAL_INTERNAL_CODEC_H

#include <stddef.h>
#include <stdint.h>

#include "gitmind/result.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Encode a CBOR payload into a base64 message string for commit bodies.
 * message_len_out (when provided) excludes the trailing null terminator.
 */
GM_NODISCARD gm_result_void_t gm_journal_encode_message(const uint8_t *cbor_data,
                                                        size_t cbor_len,
                                                        char **message_out,
                                                        size_t *message_len_out);

/*
 * Decode a commit message (base64) into binary CBOR bytes.
 * The caller owns the decoded buffer and must provide its capacity; the
 * resulting length (excluding any terminator) is returned via decoded_length.
 */
GM_NODISCARD gm_result_void_t gm_journal_decode_message(const char *raw_message,
                                                        uint8_t *decoded,
                                                        size_t decoded_capacity,
                                                        size_t *decoded_length);

#ifdef __cplusplus
}
#endif

#endif /* GITMIND_JOURNAL_INTERNAL_CODEC_H */
