/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_CBOR_COMMON_H
#define GITMIND_CBOR_COMMON_H

#include <stddef.h>
#include <stdint.h>

/* Common CBOR reading functions */
int gm_cbor_read_uint(const uint8_t *buf, size_t *offset, uint64_t *value);
int gm_cbor_read_bytes(const uint8_t *buf, size_t *offset, uint8_t *data,
                       size_t expected_len);
int gm_cbor_read_text(const uint8_t *buf, size_t *offset, char *text,
                      size_t max_len);

/* Common CBOR writing functions */
size_t gm_cbor_write_uint(uint8_t *buf, uint64_t value);
size_t gm_cbor_write_bytes(uint8_t *buf, const uint8_t *data, size_t len);
size_t gm_cbor_write_text(uint8_t *buf, const char *text);

#endif /* GITMIND_CBOR_COMMON_H */