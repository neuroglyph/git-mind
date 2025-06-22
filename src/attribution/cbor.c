/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind.h"

#include "gitmind/cbor_common.h"
#include "gitmind/constants_cbor.h"

#include <string.h>

#include "../util/gm_mem.h"

/*
 * CBOR edge encoding/decoding with strict SRP, DI, and test-double-friendly
 * design
 */

/* ========== CBOR Writer Interface (for DI) ========== */
typedef struct {
    size_t (*write_header)(uint8_t *buf);
    size_t (*write_sha)(uint8_t *buf, const uint8_t *sha);
    size_t (*write_metadata)(uint8_t *buf, uint16_t type, uint16_t conf,
                             uint64_t tstamp);
    size_t (*write_path)(uint8_t *buf, const char *path);
} gm_cbor_writer_t;

/* ========== CBOR Reader Interface (for DI) ========== */
typedef struct {
    int (*read_header)(const uint8_t *buf, size_t len, size_t *offset);
    int (*read_sha)(const uint8_t *buf, size_t *offset, uint8_t *sha);
    int (*read_type)(const uint8_t *buf, size_t *offset, uint16_t *type);
    int (*read_conf)(const uint8_t *buf, size_t *offset, uint16_t *conf);
    int (*read_tstamp)(const uint8_t *buf, size_t *offset, uint64_t *tstamp);
    int (*read_path)(const uint8_t *buf, size_t *offset, char *path);
} gm_cbor_reader_t;

/* ========== Writer Implementation (SRP: Each writes ONE thing) ========== */

static size_t write_edge_header(uint8_t *buf) {
    buf[0] = CBOR_TYPE_ARRAY | CBOR_ARRAY_SIZE_EDGE;
    return 1;
}

static size_t write_edge_sha(uint8_t *buf, const uint8_t *sha) {
    return gm_cbor_write_bytes(buf, sha, GM_SHA1_SIZE);
}

static size_t write_edge_metadata(uint8_t *buf, uint16_t type, uint16_t conf,
                                  uint64_t tstamp) {
    size_t offset = 0;
    offset += gm_cbor_write_uint(buf + offset, type);
    offset += gm_cbor_write_uint(buf + offset, conf);
    offset += gm_cbor_write_uint(buf + offset, tstamp);
    return offset;
}

static size_t write_edge_path(uint8_t *buf, const char *path) {
    return gm_cbor_write_text(buf, path);
}

/* Default writer instance */
static const gm_cbor_writer_t GM_DEFAULT_CBOR_WRITER = {
    .write_header = write_edge_header,
    .write_sha = write_edge_sha,
    .write_metadata = write_edge_metadata,
    .write_path = write_edge_path};

/* ========== Reader Implementation (SRP: Each reads ONE thing) ========== */

static int read_edge_header(const uint8_t *buf, size_t len, size_t *offset) {
    if (len < 1) {
        return GM_INVALID_ARG;
    }

    if (buf[*offset] != (CBOR_TYPE_ARRAY | CBOR_ARRAY_SIZE_EDGE)) {
        return GM_INVALID_ARG;
    }

    (*offset)++;
    return GM_OK;
}

static int read_edge_sha(const uint8_t *buf, size_t *offset, uint8_t *sha) {
    return gm_cbor_read_bytes(buf, offset, sha, GM_SHA1_SIZE);
}

/* Helper to read type and confidence separately (avoid swappable params) */
static int read_edge_type(const uint8_t *buf, size_t *offset, uint16_t *type) {
    uint64_t temp;
    if (gm_cbor_read_uint(buf, offset, &temp) != GM_OK) {
        return GM_INVALID_ARG;
    }
    *type = (uint16_t)temp;
    return GM_OK;
}

static int read_edge_confidence(const uint8_t *buf, size_t *offset,
                                uint16_t *conf) {
    uint64_t temp;
    if (gm_cbor_read_uint(buf, offset, &temp) != GM_OK) {
        return GM_INVALID_ARG;
    }
    *conf = (uint16_t)temp;
    return GM_OK;
}

static int read_edge_timestamp(const uint8_t *buf, size_t *offset,
                               uint64_t *tstamp) {
    return gm_cbor_read_uint(buf, offset, tstamp);
}

/* Removed - no longer needed with separated functions */

static int read_edge_path(const uint8_t *buf, size_t *offset, char *path) {
    return gm_cbor_read_text(buf, offset, path, GM_PATH_MAX);
}

/* Default reader instance */
static const gm_cbor_reader_t GM_DEFAULT_CBOR_READER = {
    .read_header = read_edge_header,
    .read_sha = read_edge_sha,
    .read_type = read_edge_type,
    .read_conf = read_edge_confidence,
    .read_tstamp = read_edge_timestamp,
    .read_path = read_edge_path};

/* ========== Main Encode Function (Orchestrates writing) ========== */

static int encode_with_writer(const gm_edge_t *edge, uint8_t *buffer,
                              size_t *len, const gm_cbor_writer_t *writer) {
    size_t offset = 0;

    /* Write header */
    offset += writer->write_header(buffer + offset);

    /* Write SHAs */
    offset += writer->write_sha(buffer + offset, edge->src_sha);
    offset += writer->write_sha(buffer + offset, edge->tgt_sha);

    /* Write metadata */
    offset += writer->write_metadata(buffer + offset, edge->rel_type,
                                     edge->confidence, edge->timestamp);

    /* Write paths */
    offset += writer->write_path(buffer + offset, edge->src_path);
    offset += writer->write_path(buffer + offset, edge->tgt_path);

    *len = offset;
    return GM_OK;
}

/* Public API - uses default writer */
int gm_edge_encode_cbor(const gm_edge_t *edge, uint8_t *buffer, size_t *len) {
    if (!edge || !buffer || !len) {
        return GM_INVALID_ARG;
    }

    return encode_with_writer(edge, buffer, len, &GM_DEFAULT_CBOR_WRITER);
}

/* ========== Decode Helpers (reduce complexity) ========== */

static int decode_edge_metadata(const uint8_t *buffer, size_t *offset,
                                const gm_cbor_reader_t *reader,
                                gm_edge_t *edge) {
    if (reader->read_type(buffer, offset, &edge->rel_type) != GM_OK) {
        return GM_INVALID_ARG;
    }

    if (reader->read_conf(buffer, offset, &edge->confidence) != GM_OK) {
        return GM_INVALID_ARG;
    }

    return reader->read_tstamp(buffer, offset, &edge->timestamp);
}

static int decode_edge_shas(const uint8_t *buffer, size_t *offset,
                            const gm_cbor_reader_t *reader, gm_edge_t *edge) {
    if (reader->read_sha(buffer, offset, edge->src_sha) != GM_OK) {
        return GM_INVALID_ARG;
    }

    if (reader->read_sha(buffer, offset, edge->tgt_sha) != GM_OK) {
        return GM_INVALID_ARG;
    }

    return GM_OK;
}

static int decode_edge_paths(const uint8_t *buffer, size_t *offset,
                             const gm_cbor_reader_t *reader, gm_edge_t *edge) {
    if (reader->read_path(buffer, offset, edge->src_path) != GM_OK) {
        return GM_INVALID_ARG;
    }

    if (reader->read_path(buffer, offset, edge->tgt_path) != GM_OK) {
        return GM_INVALID_ARG;
    }

    return GM_OK;
}

/* ========== Main Decode Function (Orchestrates reading) ========== */

static int decode_with_reader(const uint8_t *buffer, size_t len,
                              gm_edge_t *edge, const gm_cbor_reader_t *reader) {
    size_t offset = 0;
    gm_memset(edge, 0, sizeof(gm_edge_t));

    /* Read all components */
    if (reader->read_header(buffer, len, &offset) != GM_OK) {
        return GM_INVALID_ARG;
    }
    if (decode_edge_shas(buffer, &offset, reader, edge) != GM_OK) {
        return GM_INVALID_ARG;
    }
    if (decode_edge_metadata(buffer, &offset, reader, edge) != GM_OK) {
        return GM_INVALID_ARG;
    }
    if (decode_edge_paths(buffer, &offset, reader, edge) != GM_OK) {
        return GM_INVALID_ARG;
    }

    return GM_OK;
}

/* Public API - uses default reader */
int gm_edge_decode_cbor(const uint8_t *buffer, size_t len, gm_edge_t *edge) {
    if (!buffer || !edge || len == 0) {
        return GM_INVALID_ARG;
    }

    return decode_with_reader(buffer, len, edge, &GM_DEFAULT_CBOR_READER);
}

/* Test-double-friendly APIs (allows injection of custom reader/writer) */
int gm_edge_encode_cbor_ex(const gm_edge_t *edge, uint8_t *buffer, size_t *len,
                           const gm_cbor_writer_t *writer) {
    if (!edge || !buffer || !len || !writer) {
        return GM_INVALID_ARG;
    }

    return encode_with_writer(edge, buffer, len, writer);
}

int gm_edge_decode_cbor_ex(const uint8_t *buffer, size_t len, gm_edge_t *edge,
                           const gm_cbor_reader_t *reader) {
    if (!buffer || !edge || !reader || len == 0) {
        return GM_INVALID_ARG;
    }

    return decode_with_reader(buffer, len, edge, reader);
}