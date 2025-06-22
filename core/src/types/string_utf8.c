/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/types/string.h"
#include "gitmind/error.h"
#include "gitmind/utf8/validate.h"
#include <string.h>

/* Validate UTF-8 */
gm_result_void gm_string_validate_utf8(const gm_string_t* str) {
    if (!str) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT, "NULL string"));
    }
    
    /* Use DFA-based UTF-8 validator */
    gm_utf8_error_t utf8_err = gm_utf8_validate(str->data, str->length);
    
    if (utf8_err != GM_UTF8_OK) {
        const char* err_msg;
        switch (utf8_err) {
            case GM_UTF8_ERR_OVERLONG:
                err_msg = "Overlong UTF-8 encoding detected";
                break;
            case GM_UTF8_ERR_INVALID_START:
                err_msg = "Invalid UTF-8 start byte";
                break;
            case GM_UTF8_ERR_TRUNCATED:
                err_msg = "Truncated UTF-8 sequence";
                break;
            case GM_UTF8_ERR_SURROGATE:
                err_msg = "UTF-16 surrogate in UTF-8";
                break;
            case GM_UTF8_ERR_OUT_OF_RANGE:
                err_msg = "Codepoint out of Unicode range";
                break;
            default:
                err_msg = "Invalid UTF-8 encoding";
                break;
        }
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_UTF8, err_msg));
    }
    
    return gm_ok_void();
}