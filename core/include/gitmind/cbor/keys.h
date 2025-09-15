/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_CBOR_KEYS_H
#define GITMIND_CBOR_KEYS_H

/* Common CBOR keys for edge encodings */

/* Legacy/basic edge fields */
#define GM_CBOR_KEY_SRC_SHA      0
#define GM_CBOR_KEY_TGT_SHA      1
#define GM_CBOR_KEY_REL_TYPE     2
#define GM_CBOR_KEY_CONFIDENCE   3
#define GM_CBOR_KEY_TIMESTAMP    4
#define GM_CBOR_KEY_SRC_PATH     5
#define GM_CBOR_KEY_TGT_PATH     6
#define GM_CBOR_KEY_ULID         7

/* OID-first additions (raw bytes of length GM_OID_RAWSZ) */
#define GM_CBOR_KEY_SRC_OID      8
#define GM_CBOR_KEY_TGT_OID      9

/* Attributed extras */
#define GM_CBOR_KEY_SOURCE_TYPE  10
#define GM_CBOR_KEY_AUTHOR       11
#define GM_CBOR_KEY_SESSION      12
#define GM_CBOR_KEY_FLAGS        13
#define GM_CBOR_KEY_LANE         14

/* Field counts for writers */
#define GM_CBOR_EDGE_FIELDS_TOTAL       10  /* legacy 8 + 2 OID fields */
#define GM_CBOR_ATTR_EDGE_FIELDS_TOTAL  15  /* edge total + 5 attribution fields */

#endif /* GITMIND_CBOR_KEYS_H */

