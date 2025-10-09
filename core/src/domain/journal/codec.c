/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/journal/internal/codec.h"

#include <sodium/utils.h>

#include <stdlib.h>
#include <string.h>

#include "gitmind/error.h"
#include "gitmind/result.h"

GM_NODISCARD gm_result_void_t gm_journal_encode_message(const uint8_t *cbor_data,
                                                        size_t cbor_len,
                                                        char **message_out,
                                                        size_t *message_len_out) {
    if (cbor_data == NULL || cbor_len == 0U || message_out == NULL) {
        return gm_err_void(
            GM_ERROR(GM_ERR_INVALID_ARGUMENT, "journal encode requires input"));
    }
    const int variant = sodium_base64_VARIANT_ORIGINAL;
    size_t required = sodium_base64_ENCODED_LEN(cbor_len, variant);
    char *encoded = (char *)malloc(required);
    if (encoded == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_OUT_OF_MEMORY, "alloc b64 buffer"));
    }
    sodium_bin2base64(encoded, required, cbor_data, cbor_len, variant);
    if (required > 0U) {
        encoded[required - 1U] = '\0';
    }
    *message_out = encoded;
    if (message_len_out != NULL) {
        *message_len_out = required;
    }
    return gm_ok_void();
}

GM_NODISCARD gm_result_void_t gm_journal_decode_message(const char *raw_message,
                                                        uint8_t *decoded,
                                                        size_t *decoded_length) {
    if (raw_message == NULL || decoded == NULL || decoded_length == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT, "decode requires buffers"));
    }
    const size_t raw_length = strlen(raw_message);
    const int variant = sodium_base64_VARIANT_ORIGINAL;
    size_t out_len = 0;
    if (sodium_base642bin(decoded, *decoded_length, raw_message, raw_length,
                          NULL, &out_len, NULL, variant) != 0) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_FORMAT, "invalid base64"));
    }
    *decoded_length = out_len;
    return gm_ok_void();
}

