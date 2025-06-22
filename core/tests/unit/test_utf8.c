/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/utf8/validate.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Test valid ASCII */
static void test_utf8_ascii(void) {
    const char* valid[] = {
        "hello",
        "Hello, World!",
        "1234567890",
        "!@#$%^&*()_+-=",
        ""  /* Empty string is valid */
    };
    
    for (size_t i = 0; i < sizeof(valid) / sizeof(valid[0]); i++) {
        gm_utf8_error_t err = gm_utf8_validate(valid[i], strlen(valid[i]));
        assert(err == GM_UTF8_OK);
    }
    
    printf("✓ test_utf8_ascii\n");
}

/* Test valid multibyte sequences */
static void test_utf8_multibyte(void) {
    /* Valid UTF-8 sequences */
    const struct {
        const char* data;
        size_t len;
        const char* desc;
    } valid[] = {
        /* 2-byte sequences */
        { "\xC2\x80", 2, "U+0080" },                    /* First 2-byte */
        { "\xDF\xBF", 2, "U+07FF" },                    /* Last 2-byte */
        
        /* 3-byte sequences */
        { "\xE0\xA0\x80", 3, "U+0800" },                /* First 3-byte */
        { "\xE0\xBF\xBF", 3, "U+0FFF" },                /* Valid 3-byte */
        { "\xED\x9F\xBF", 3, "U+D7FF" },                /* Before surrogates */
        { "\xEE\x80\x80", 3, "U+E000" },                /* After surrogates */
        { "\xEF\xBF\xBF", 3, "U+FFFF" },                /* Last 3-byte */
        
        /* 4-byte sequences */
        { "\xF0\x90\x80\x80", 4, "U+10000" },           /* First 4-byte */
        { "\xF4\x8F\xBF\xBF", 4, "U+10FFFF" },          /* Last valid Unicode */
        
        /* Mixed sequences */
        { "a\xC2\x80" "b", 4, "Mixed ASCII and 2-byte" },
        { "\xE2\x82\xAC\xE2\x82\xAC", 6, "€€" },        /* Two euro signs */
        { "Hello, \xE4\xB8\x96\xE7\x95\x8C!", 13, "Hello, 世界!" }
    };
    
    for (size_t i = 0; i < sizeof(valid) / sizeof(valid[0]); i++) {
        gm_utf8_error_t err = gm_utf8_validate(valid[i].data, valid[i].len);
        if (err != GM_UTF8_OK) {
            printf("❌ Failed on valid sequence: %s (error %d)\n", 
                   valid[i].desc, err);
        }
        assert(err == GM_UTF8_OK);
    }
    
    printf("✓ test_utf8_multibyte\n");
}

/* Test overlong encodings */
static void test_utf8_overlong(void) {
    const struct {
        const char* data;
        size_t len;
        const char* desc;
    } overlong[] = {
        /* Overlong NULL */
        { "\xC0\x80", 2, "Overlong U+0000" },
        { "\xE0\x80\x80", 3, "Overlong U+0000" },
        { "\xF0\x80\x80\x80", 4, "Overlong U+0000" },
        
        /* Other overlong sequences */
        { "\xC0\xAF", 2, "Overlong slash" },
        { "\xE0\x80\xAF", 3, "Overlong slash" },
        { "\xF0\x80\x80\xAF", 4, "Overlong slash" },
        
        /* Overlong 2-byte */
        { "\xC1\xBF", 2, "Overlong U+007F" }
    };
    
    for (size_t i = 0; i < sizeof(overlong) / sizeof(overlong[0]); i++) {
        gm_utf8_error_t err = gm_utf8_validate(overlong[i].data, overlong[i].len);
        if (err == GM_UTF8_OK) {
            printf("❌ Failed to reject overlong: %s\n", overlong[i].desc);
        }
        assert(err != GM_UTF8_OK);
    }
    
    printf("✓ test_utf8_overlong\n");
}

/* Test invalid start bytes */
static void test_utf8_invalid_start(void) {
    const struct {
        const char* data;
        size_t len;
        const char* desc;
    } invalid[] = {
        /* Invalid start bytes */
        { "\x80", 1, "Continuation byte as start" },
        { "\xBF", 1, "Continuation byte as start" },
        { "\xFE", 1, "Invalid 0xFE" },
        { "\xFF", 1, "Invalid 0xFF" },
        
        /* Invalid in sequences */
        { "\xF5\x80\x80\x80", 4, "F5 start byte" },
        { "\xF8\x80\x80\x80", 4, "F8 start byte" },
        { "\xFC\x80\x80\x80\x80", 5, "FC start byte" }
    };
    
    for (size_t i = 0; i < sizeof(invalid) / sizeof(invalid[0]); i++) {
        gm_utf8_error_t err = gm_utf8_validate(invalid[i].data, invalid[i].len);
        if (err == GM_UTF8_OK) {
            printf("❌ Failed to reject invalid start: %s\n", invalid[i].desc);
        }
        assert(err == GM_UTF8_ERR_INVALID_START);
    }
    
    printf("✓ test_utf8_invalid_start\n");
}

/* Test truncated sequences */
static void test_utf8_truncated(void) {
    const struct {
        const char* data;
        size_t len;
        const char* desc;
    } truncated[] = {
        /* Missing continuation bytes */
        { "\xC2", 1, "2-byte missing 1" },
        { "\xE0", 1, "3-byte missing 2" },
        { "\xE0\xA0", 2, "3-byte missing 1" },
        { "\xF0", 1, "4-byte missing 3" },
        { "\xF0\x90", 2, "4-byte missing 2" },
        { "\xF0\x90\x80", 3, "4-byte missing 1" },
        
        /* Truncated in middle of string */
        { "Hello\xC2", 6, "Truncated after ASCII" },
        { "Test\xE0\xA0", 6, "Truncated 3-byte" }
    };
    
    for (size_t i = 0; i < sizeof(truncated) / sizeof(truncated[0]); i++) {
        gm_utf8_error_t err = gm_utf8_validate(truncated[i].data, truncated[i].len);
        if (err != GM_UTF8_ERR_TRUNCATED) {
            printf("❌ Failed to detect truncated: %s (got %d)\n", 
                   truncated[i].desc, err);
        }
        assert(err == GM_UTF8_ERR_TRUNCATED);
    }
    
    printf("✓ test_utf8_truncated\n");
}

/* Test UTF-16 surrogates */
static void test_utf8_surrogates(void) {
    const struct {
        const char* data;
        size_t len;
        const char* desc;
    } surrogates[] = {
        /* UTF-16 surrogates (D800-DFFF) are invalid in UTF-8 */
        { "\xED\xA0\x80", 3, "U+D800 (first surrogate)" },
        { "\xED\xAF\xBF", 3, "U+DBFF (last high surrogate)" },
        { "\xED\xB0\x80", 3, "U+DC00 (first low surrogate)" },
        { "\xED\xBF\xBF", 3, "U+DFFF (last surrogate)" },
        { "\xED\xA5\x8C", 3, "U+D94C (random surrogate)" }
    };
    
    for (size_t i = 0; i < sizeof(surrogates) / sizeof(surrogates[0]); i++) {
        gm_utf8_error_t err = gm_utf8_validate(surrogates[i].data, surrogates[i].len);
        /* The DFA rejects surrogates as invalid sequences (ERR_INVALID_START)
           This is correct - surrogates are detected at the byte level */
        if (err == GM_UTF8_OK) {
            printf("❌ Failed to reject surrogate: %s\n", surrogates[i].desc);
        }
        assert(err != GM_UTF8_OK);
    }
    
    printf("✓ test_utf8_surrogates\n");
}

/* Test out of range codepoints */
static void test_utf8_out_of_range(void) {
    const struct {
        const char* data;
        size_t len;
        const char* desc;
    } out_of_range[] = {
        /* Beyond U+10FFFF */
        { "\xF4\x90\x80\x80", 4, "U+110000" },
        { "\xF4\x90\x80\x81", 4, "U+110001" },
        { "\xF5\x80\x80\x80", 4, "U+140000" },
        { "\xF7\xBF\xBF\xBF", 4, "U+1FFFFF" }
    };
    
    for (size_t i = 0; i < sizeof(out_of_range) / sizeof(out_of_range[0]); i++) {
        gm_utf8_error_t err = gm_utf8_validate(out_of_range[i].data, 
                                               out_of_range[i].len);
        if (err != GM_UTF8_ERR_OUT_OF_RANGE && err != GM_UTF8_ERR_INVALID_START) {
            printf("❌ Failed to reject out of range: %s (got %d)\n", 
                   out_of_range[i].desc, err);
        }
        assert(err == GM_UTF8_ERR_OUT_OF_RANGE || err == GM_UTF8_ERR_INVALID_START);
    }
    
    printf("✓ test_utf8_out_of_range\n");
}

/* Test streaming validation */
static void test_utf8_streaming(void) {
    /* Test 1: Valid multi-chunk */
    {
        gm_utf8_state_t state;
        gm_utf8_state_init(&state);
        
        assert(gm_utf8_validate_chunk(&state, "Hello", 5) == GM_UTF8_OK);
        assert(gm_utf8_validate_chunk(&state, ", ", 2) == GM_UTF8_OK);
        assert(gm_utf8_validate_chunk(&state, "\xE4\xB8\x96\xE7\x95\x8C", 6) == GM_UTF8_OK);
        assert(gm_utf8_validate_chunk(&state, "!", 1) == GM_UTF8_OK);
        
        assert(gm_utf8_state_is_complete(&state));
    }
    
    /* Test 2: Split multibyte sequence */
    {
        gm_utf8_state_t state;
        gm_utf8_state_init(&state);
        
        /* Euro sign split across chunks */
        assert(gm_utf8_validate_chunk(&state, "\xE2", 1) == GM_UTF8_OK);
        assert(!gm_utf8_state_is_complete(&state)); /* Not complete */
        assert(gm_utf8_validate_chunk(&state, "\x82", 1) == GM_UTF8_OK);
        assert(!gm_utf8_state_is_complete(&state)); /* Still not complete */
        assert(gm_utf8_validate_chunk(&state, "\xAC", 1) == GM_UTF8_OK);
        assert(gm_utf8_state_is_complete(&state));  /* Now complete */
    }
    
    /* Test 3: Error in second chunk */
    {
        gm_utf8_state_t state;
        gm_utf8_state_init(&state);
        
        assert(gm_utf8_validate_chunk(&state, "abc", 3) == GM_UTF8_OK);
        assert(gm_utf8_validate_chunk(&state, "\xC0\x80", 2) != GM_UTF8_OK);
    }
    
    printf("✓ test_utf8_streaming\n");
}

/* Performance test (basic) */
static void test_utf8_performance(void) {
    /* Generate 1MB of ASCII data */
    size_t size = 1024 * 1024;
    char* buf = malloc(size);
    assert(buf != NULL);
    
    /* Fill with ASCII */
    for (size_t i = 0; i < size; i++) {
        buf[i] = 'A' + (i % 26);
    }
    
    /* Time the validation (rough estimate) */
    gm_utf8_error_t err = gm_utf8_validate(buf, size);
    assert(err == GM_UTF8_OK);
    
    free(buf);
    
    printf("✓ test_utf8_performance (validated 1MB)\n");
}

int main(void) {
    printf("Running UTF-8 validation tests...\n\n");
    
    test_utf8_ascii();
    test_utf8_multibyte();
    test_utf8_overlong();
    test_utf8_invalid_start();
    test_utf8_truncated();
    test_utf8_surrogates();
    test_utf8_out_of_range();
    test_utf8_streaming();
    test_utf8_performance();
    
    printf("\n✅ All UTF-8 tests passed!\n");
    return 0;
}