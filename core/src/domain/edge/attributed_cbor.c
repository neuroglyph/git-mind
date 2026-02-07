/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/domain/edge/attributed_cbor.h"

#include "gitmind/edge_attributed.h"
#include "gitmind/cbor/cbor.h"
#include "gitmind/cbor/keys.h"
#include "gitmind/error.h"
#include "gitmind/result.h"
#include "gitmind/util/oid.h"
#include "gitmind/security/memory.h"

#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint8_t *buffer;
    size_t capacity;
    size_t offset;
} edge_cbor_writer_t;

static size_t writer_remaining(const edge_cbor_writer_t *writer) {
    return (writer->capacity > writer->offset)
               ? (writer->capacity - writer->offset)
               : 0U;
}

static gm_result_void_t writer_write_uint(edge_cbor_writer_t *writer,
                                          uint64_t value) {
    gm_result_size_t result = gm_cbor_write_uint(
        value, writer->buffer + writer->offset, writer_remaining(writer));
    if (!result.ok) {
        return gm_err_void(result.u.err);
    }
    writer->offset += result.u.val;
    return gm_ok_void();
}

static gm_result_void_t writer_write_bytes(edge_cbor_writer_t *writer,
                                           const uint8_t *bytes,
                                           size_t length) {
    gm_result_size_t result =
        gm_cbor_write_bytes(writer->buffer + writer->offset,
                            writer_remaining(writer), bytes, length);
    if (!result.ok) {
        return gm_err_void(result.u.err);
    }
    writer->offset += result.u.val;
    return gm_ok_void();
}

static gm_result_void_t writer_write_text(edge_cbor_writer_t *writer,
                                          const char *text) {
    gm_result_size_t result =
        gm_cbor_write_text(writer->buffer + writer->offset,
                           writer_remaining(writer), text);
    if (!result.ok) {
        return gm_err_void(result.u.err);
    }
    writer->offset += result.u.val;
    return gm_ok_void();
}

gm_result_void_t gm_edge_attributed_cbor_encode(
    const struct gm_edge_attributed *edge,
    uint8_t *buffer,
    size_t *len) {
    if (edge == NULL || buffer == NULL || len == NULL) {
        return gm_err_void(
            GM_ERROR(GM_ERR_INVALID_ARGUMENT, "edge encode arguments null"));
    }

    edge_cbor_writer_t writer = {
        .buffer = buffer,
        .capacity = *len,
        .offset = 0U,
    };

    if (writer_remaining(&writer) < 1U) {
        return gm_err_void(
            GM_ERROR(GM_ERR_BUFFER_TOO_SMALL, "insufficient buffer space"));
    }
    buffer[writer.offset++] =
        (uint8_t)(0xA0 | (GM_CBOR_ATTR_EDGE_FIELDS_TOTAL & 0x1FU));

    const uint8_t *src_oid_bytes =
        gm_oid_is_zero(&edge->src_oid) ? edge->src_sha
                                       : (const uint8_t *)edge->src_oid.id;
    const uint8_t *tgt_oid_bytes =
        gm_oid_is_zero(&edge->tgt_oid) ? edge->tgt_sha
                                       : (const uint8_t *)edge->tgt_oid.id;

    GM_TRY(writer_write_uint(&writer, GM_CBOR_KEY_SRC_SHA));
    GM_TRY(writer_write_bytes(&writer, edge->src_sha, GM_SHA1_SIZE));

    GM_TRY(writer_write_uint(&writer, GM_CBOR_KEY_TGT_SHA));
    GM_TRY(writer_write_bytes(&writer, edge->tgt_sha, GM_SHA1_SIZE));

    GM_TRY(writer_write_uint(&writer, GM_CBOR_KEY_REL_TYPE));
    GM_TRY(writer_write_uint(&writer, edge->rel_type));

    GM_TRY(writer_write_uint(&writer, GM_CBOR_KEY_CONFIDENCE));
    GM_TRY(writer_write_uint(&writer, edge->confidence));

    GM_TRY(writer_write_uint(&writer, GM_CBOR_KEY_TIMESTAMP));
    GM_TRY(writer_write_uint(&writer, edge->timestamp));

    GM_TRY(writer_write_uint(&writer, GM_CBOR_KEY_SRC_PATH));
    GM_TRY(writer_write_text(&writer, edge->src_path));

    GM_TRY(writer_write_uint(&writer, GM_CBOR_KEY_TGT_PATH));
    GM_TRY(writer_write_text(&writer, edge->tgt_path));

    GM_TRY(writer_write_uint(&writer, GM_CBOR_KEY_ULID));
    GM_TRY(writer_write_text(&writer, edge->ulid));

    GM_TRY(writer_write_uint(&writer, GM_CBOR_KEY_SRC_OID));
    GM_TRY(writer_write_bytes(&writer, src_oid_bytes, GM_OID_RAWSZ));

    GM_TRY(writer_write_uint(&writer, GM_CBOR_KEY_TGT_OID));
    GM_TRY(writer_write_bytes(&writer, tgt_oid_bytes, GM_OID_RAWSZ));

    GM_TRY(writer_write_uint(&writer, GM_CBOR_KEY_SOURCE_TYPE));
    GM_TRY(writer_write_uint(&writer,
                             (uint64_t)edge->attribution.source_type));

    GM_TRY(writer_write_uint(&writer, GM_CBOR_KEY_AUTHOR));
    GM_TRY(writer_write_text(&writer, edge->attribution.author));

    GM_TRY(writer_write_uint(&writer, GM_CBOR_KEY_SESSION));
    GM_TRY(writer_write_text(&writer, edge->attribution.session_id));

    GM_TRY(writer_write_uint(&writer, GM_CBOR_KEY_FLAGS));
    GM_TRY(writer_write_uint(&writer, edge->attribution.flags));

    GM_TRY(writer_write_uint(&writer, GM_CBOR_KEY_LANE));
    GM_TRY(writer_write_uint(&writer, (uint64_t)edge->lane));

    *len = writer.offset;
    return gm_ok_void();
}

typedef struct {
    const uint8_t *buffer;
    size_t size;
    size_t offset;
} edge_cbor_reader_t;

static gm_result_uint64_t reader_read_uint(edge_cbor_reader_t *reader) {
    return gm_cbor_read_uint(reader->buffer, &reader->offset, reader->size);
}

static gm_result_void_t reader_read_bytes(edge_cbor_reader_t *reader,
                                          uint8_t *out,
                                          size_t length) {
    return gm_cbor_read_bytes(reader->buffer, &reader->offset, reader->size,
                              out, length);
}

static gm_result_void_t reader_read_text(edge_cbor_reader_t *reader,
                                         char *out,
                                         size_t length) {
    return gm_cbor_read_text(reader->buffer, &reader->offset, reader->size,
                             out, length);
}

typedef gm_result_void_t (*edge_field_decoder_fn)(edge_cbor_reader_t *,
                                                  struct gm_edge_attributed *);

static gm_result_void_t decode_src_sha(edge_cbor_reader_t *reader,
                                       struct gm_edge_attributed *edge) {
    return reader_read_bytes(reader, edge->src_sha, GM_SHA1_SIZE);
}

static gm_result_void_t decode_tgt_sha(edge_cbor_reader_t *reader,
                                       struct gm_edge_attributed *edge) {
    return reader_read_bytes(reader, edge->tgt_sha, GM_SHA1_SIZE);
}

static gm_result_void_t decode_rel_type(edge_cbor_reader_t *reader,
                                        struct gm_edge_attributed *edge) {
    gm_result_uint64_t value = reader_read_uint(reader);
    if (!value.ok) {
        return gm_err_void(value.u.err);
    }
    edge->rel_type = (uint16_t)value.u.val;
    return gm_ok_void();
}

static gm_result_void_t decode_confidence(edge_cbor_reader_t *reader,
                                          struct gm_edge_attributed *edge) {
    gm_result_uint64_t value = reader_read_uint(reader);
    if (!value.ok) {
        return gm_err_void(value.u.err);
    }
    edge->confidence = (uint16_t)value.u.val;
    return gm_ok_void();
}

static gm_result_void_t decode_timestamp(edge_cbor_reader_t *reader,
                                         struct gm_edge_attributed *edge) {
    gm_result_uint64_t value = reader_read_uint(reader);
    if (!value.ok) {
        return gm_err_void(value.u.err);
    }
    edge->timestamp = value.u.val;
    return gm_ok_void();
}

static gm_result_void_t decode_src_path(edge_cbor_reader_t *reader,
                                        struct gm_edge_attributed *edge) {
    return reader_read_text(reader, edge->src_path, GM_PATH_MAX);
}

static gm_result_void_t decode_tgt_path(edge_cbor_reader_t *reader,
                                        struct gm_edge_attributed *edge) {
    return reader_read_text(reader, edge->tgt_path, GM_PATH_MAX);
}

static gm_result_void_t decode_ulid(edge_cbor_reader_t *reader,
                                    struct gm_edge_attributed *edge) {
    return reader_read_text(reader, edge->ulid, GM_ULID_SIZE + 1U);
}

static gm_result_void_t decode_src_oid(edge_cbor_reader_t *reader,
                                       struct gm_edge_attributed *edge) {
    uint8_t raw[GM_OID_RAWSZ];
    gm_result_void_t result =
        reader_read_bytes(reader, raw, GM_OID_RAWSZ);
    if (!result.ok) {
        return result;
    }
    if (gm_oid_from_raw(&edge->src_oid, raw, GM_OID_RAWSZ) != GM_OK) {
        return gm_err_void(
            GM_ERROR(GM_ERR_INVALID_FORMAT, "invalid src oid"));
    }
    return gm_ok_void();
}

static gm_result_void_t decode_tgt_oid(edge_cbor_reader_t *reader,
                                       struct gm_edge_attributed *edge) {
    uint8_t raw[GM_OID_RAWSZ];
    gm_result_void_t result =
        reader_read_bytes(reader, raw, GM_OID_RAWSZ);
    if (!result.ok) {
        return result;
    }
    if (gm_oid_from_raw(&edge->tgt_oid, raw, GM_OID_RAWSZ) != GM_OK) {
        return gm_err_void(
            GM_ERROR(GM_ERR_INVALID_FORMAT, "invalid tgt oid"));
    }
    return gm_ok_void();
}

static gm_result_void_t decode_source_type(edge_cbor_reader_t *reader,
                                           struct gm_edge_attributed *edge) {
    gm_result_uint64_t value = reader_read_uint(reader);
    if (!value.ok) {
        return gm_err_void(value.u.err);
    }
    edge->attribution.source_type = (gm_source_type_t)value.u.val;
    return gm_ok_void();
}

static gm_result_void_t decode_author(edge_cbor_reader_t *reader,
                                      struct gm_edge_attributed *edge) {
    return reader_read_text(reader, edge->attribution.author,
                            sizeof edge->attribution.author);
}

static gm_result_void_t decode_session(edge_cbor_reader_t *reader,
                                       struct gm_edge_attributed *edge) {
    return reader_read_text(reader, edge->attribution.session_id,
                            sizeof edge->attribution.session_id);
}

static gm_result_void_t decode_flags(edge_cbor_reader_t *reader,
                                     struct gm_edge_attributed *edge) {
    gm_result_uint64_t value = reader_read_uint(reader);
    if (!value.ok) {
        return gm_err_void(value.u.err);
    }
    edge->attribution.flags = (uint32_t)value.u.val;
    return gm_ok_void();
}

static gm_result_void_t decode_lane(edge_cbor_reader_t *reader,
                                    struct gm_edge_attributed *edge) {
    gm_result_uint64_t value = reader_read_uint(reader);
    if (!value.ok) {
        return gm_err_void(value.u.err);
    }
    edge->lane = (gm_lane_type_t)value.u.val;
    return gm_ok_void();
}

static const struct {
    uint64_t key;
    edge_field_decoder_fn decoder;
} kFieldDecoders[] = {
    {GM_CBOR_KEY_SRC_SHA, decode_src_sha},
    {GM_CBOR_KEY_TGT_SHA, decode_tgt_sha},
    {GM_CBOR_KEY_REL_TYPE, decode_rel_type},
    {GM_CBOR_KEY_CONFIDENCE, decode_confidence},
    {GM_CBOR_KEY_TIMESTAMP, decode_timestamp},
    {GM_CBOR_KEY_SRC_PATH, decode_src_path},
    {GM_CBOR_KEY_TGT_PATH, decode_tgt_path},
    {GM_CBOR_KEY_ULID, decode_ulid},
    {GM_CBOR_KEY_SRC_OID, decode_src_oid},
    {GM_CBOR_KEY_TGT_OID, decode_tgt_oid},
    {GM_CBOR_KEY_SOURCE_TYPE, decode_source_type},
    {GM_CBOR_KEY_AUTHOR, decode_author},
    {GM_CBOR_KEY_SESSION, decode_session},
    {GM_CBOR_KEY_FLAGS, decode_flags},
    {GM_CBOR_KEY_LANE, decode_lane},
};

static edge_field_decoder_fn find_decoder(uint64_t key) {
    for (size_t i = 0; i < (sizeof(kFieldDecoders) / sizeof(kFieldDecoders[0]));
         ++i) {
        if (kFieldDecoders[i].key == key) {
            return kFieldDecoders[i].decoder;
        }
    }
    return NULL;
}

gm_result_void_t gm_edge_attributed_cbor_decode(
    const uint8_t *buffer,
    size_t len,
    struct gm_edge_attributed *edge_out,
    size_t *consumed) {
    if (buffer == NULL || edge_out == NULL || consumed == NULL) {
        return gm_err_void(
            GM_ERROR(GM_ERR_INVALID_ARGUMENT, "edge decode arguments null"));
    }
    if (len == 0U) {
        return gm_err_void(
            GM_ERROR(GM_ERR_INVALID_FORMAT, "empty CBOR buffer"));
    }

    edge_cbor_reader_t reader = {
        .buffer = buffer,
        .size = len,
        .offset = 0U,
    };

    uint8_t initial = buffer[reader.offset++];
    if ((initial & 0xE0U) != 0xA0U) {
        return gm_err_void(
            GM_ERROR(GM_ERR_INVALID_FORMAT, "edge map header missing"));
    }
    uint8_t additional = (uint8_t)(initial & 0x1FU);
    if (additional >= 24U) {
        return gm_err_void(
            GM_ERROR(GM_ERR_INVALID_FORMAT, "edge map size invalid"));
    }
    uint32_t fields = additional;

    gm_edge_attributed_t decoded_edge;
    gm_memset_safe(&decoded_edge, sizeof(decoded_edge), 0, sizeof(decoded_edge));

    for (uint32_t i = 0; i < fields; ++i) {
        gm_result_uint64_t key_result = reader_read_uint(&reader);
        if (!key_result.ok) {
            return gm_err_void(key_result.u.err);
        }
        edge_field_decoder_fn decoder = find_decoder(key_result.u.val);
        if (decoder == NULL) {
            return gm_err_void(
                GM_ERROR(GM_ERR_INVALID_FORMAT, "unknown edge field key"));
        }
        gm_result_void_t decode_result = decoder(&reader, &decoded_edge);
        if (!decode_result.ok) {
            return decode_result;
        }
    }

    if (gm_oid_is_zero(&decoded_edge.src_oid)) {
        (void)gm_oid_from_raw(&decoded_edge.src_oid, decoded_edge.src_sha,
                              GM_OID_RAWSZ);
    }
    if (gm_oid_is_zero(&decoded_edge.tgt_oid)) {
        (void)gm_oid_from_raw(&decoded_edge.tgt_oid, decoded_edge.tgt_sha,
                              GM_OID_RAWSZ);
    }

    *edge_out = decoded_edge;
    *consumed = reader.offset;
    return gm_ok_void();
}
