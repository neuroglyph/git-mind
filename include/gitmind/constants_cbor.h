/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_CONSTANTS_CBOR_H
#define GITMIND_CONSTANTS_CBOR_H

/* CBOR Major Types (RFC 7049) */
#define CBOR_TYPE_UNSIGNED      0x00
#define CBOR_TYPE_NEGATIVE      0x20
#define CBOR_TYPE_BYTES         0x40
#define CBOR_TYPE_TEXT          0x60
#define CBOR_TYPE_ARRAY         0x80
#define CBOR_TYPE_MAP           0xA0
#define CBOR_TYPE_TAG           0xC0
#define CBOR_TYPE_SIMPLE        0xE0

/* CBOR Type Masks */
#define CBOR_TYPE_MASK          0xE0
#define CBOR_ADDITIONAL_INFO_MASK   0x1F

/* CBOR Additional Information Values */
#define CBOR_MAX_IMMEDIATE      0x17
#define CBOR_IMMEDIATE_THRESHOLD    24
#define CBOR_UINT8_FOLLOWS      0x18
#define CBOR_UINT16_FOLLOWS     0x19
#define CBOR_UINT32_FOLLOWS     0x1A
#define CBOR_UINT64_FOLLOWS     0x1B
#define CBOR_RESERVED_28        0x1C
#define CBOR_RESERVED_29        0x1D
#define CBOR_RESERVED_30        0x1E
#define CBOR_INDEFINITE         0x1F

/* CBOR Array Sizes for git-mind */
#define CBOR_ARRAY_SIZE_ATTRIBUTED  13  /* Attributed edge format */
#define CBOR_ARRAY_SIZE_LEGACY      8   /* Legacy edge format */
#define CBOR_ARRAY_SIZE_EDGE        7   /* Basic edge format */

/* Field Sizes */
#define CBOR_SHA_SIZE           20      /* Git SHA1 size in bytes */
#define CBOR_ULID_SIZE          26      /* ULID size in bytes */
#define CBOR_TIMESTAMP_SIZE     8       /* Unix timestamp size */

/* Field Limits */
#define CBOR_MAX_STRING_LENGTH  65536   /* Maximum string field length */
#define CBOR_MAX_PATH_LENGTH    4096    /* Maximum path length */
#define CBOR_MAX_TYPE_LENGTH    64      /* Maximum relationship type length */
#define CBOR_MAX_SOURCE_LENGTH  32      /* Maximum source identifier length */
#define CBOR_MAX_AUTHOR_LENGTH  256     /* Maximum author string length */
#define CBOR_MAX_SESSION_LENGTH 256     /* Maximum session ID length */
#define CBOR_MAX_LANE_LENGTH    32      /* Maximum lane name length */

/* Bit Shift Constants */
#define SHIFT_8                 8
#define SHIFT_16                16
#define SHIFT_24                24
#define SHIFT_32                32
#define SHIFT_40                40
#define SHIFT_48                48
#define SHIFT_56                56

/* Byte Operations */
#define BYTE_SIZE               8
#define BYTE_MASK               0xFF

/* Endianness Helpers */
#define CBOR_ENCODE_UINT16(x)   ((((x) >> 8) & 0xFF) | (((x) & 0xFF) << 8))
#define CBOR_ENCODE_UINT32(x)   ((((x) >> 24) & 0xFF) | (((x) >> 8) & 0xFF00) | \
                                 (((x) << 8) & 0xFF0000) | (((x) << 24) & 0xFF000000))

#endif /* GITMIND_CONSTANTS_CBOR_H */