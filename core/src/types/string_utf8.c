/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/error.h"
#include "gitmind/result.h"
#include "gitmind/types/string.h"
#include "gitmind/utf8/validate.h"

/* Helper to map UTF-8 error codes to messages */
static const char *utf8_error_to_message(gm_utf8_error_t utf8_err) {
    switch (utf8_err) {
    case GM_UTF8_ERR_OVERLONG:
        return "Overlong UTF-8 encoding detected";
    case GM_UTF8_ERR_INVALID_START:
        return "Invalid UTF-8 start byte";
    case GM_UTF8_ERR_TRUNCATED:
        return "Truncated UTF-8 sequence";
    case GM_UTF8_ERR_SURROGATE:
        return "UTF-16 surrogate in UTF-8";
    case GM_UTF8_ERR_OUT_OF_RANGE:
        return "Codepoint out of Unicode range";
    default:
        return "Invalid UTF-8 encoding";
    }
}

/* Validate UTF-8 */
gm_result_void_t gm_string_validate_utf8(const gm_string_t *str) {
    if (!str) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT, "nullptr string"));
    }

    gm_utf8_error_t utf8_err = gm_utf8_validate(str->data, str->length);
    if (utf8_err != GM_UTF8_OK) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_UTF8, utf8_error_to_message(utf8_err)));
    }

    return gm_ok_void();
}
