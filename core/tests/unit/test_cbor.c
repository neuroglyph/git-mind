/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */

#include <gitmind/cbor/cbor.h>
#include <gitmind/cbor/constants_cbor.h>
#include <gitmind/error.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

/* Test constants */
#define BUFFER_SIZE 1024

/* Test helper to verify error codes */
static void assert_error_code(gm_error_t *err, int expected_code) {
    assert(err != NULL);
    assert(err->code == expected_code);
    gm_error_free(err);
}

/* Test unsigned integer encoding and decoding */
static void test_cbor_uint(void) {
    printf("test_cbor_uint... ");
    
    uint8_t buffer[BUFFER_SIZE];
    size_t offset;
    
    /* Test cases with expected encoding sizes */
    struct {
        uint64_t value;
        size_t expected_size;
    } test_cases[] = {
        {0, 1},                  /* Immediate value 0 */
        {23, 1},                 /* Immediate value 23 */
        {24, 2},                 /* uint8 follows */
        {255, 2},                /* uint8 max */
        {256, 3},                /* uint16 follows */
        {65535, 3},              /* uint16 max */
        {65536, 5},              /* uint32 follows */
        {4294967295UL, 5},       /* uint32 max */
        {4294967296UL, 9},       /* uint64 follows */
        {UINT64_MAX, 9},         /* uint64 max */
    };
    
    for (size_t i = 0; i < sizeof(test_cases) / sizeof(test_cases[0]); i++) {
        uint64_t value = test_cases[i].value;
        size_t expected_size = test_cases[i].expected_size;
        
        /* Write value */
        gm_result_size_t write_result = gm_cbor_write_uint(buffer, BUFFER_SIZE, value);
        assert(write_result.ok);
        assert(write_result.u.val == expected_size);
        
        /* Read value back */
        offset = 0;
        gm_result_uint64_t read_result = gm_cbor_read_uint(buffer, &offset, BUFFER_SIZE);
        assert(read_result.ok);
        assert(read_result.u.val == value);
        assert(offset == expected_size);
    }
    
    /* Test buffer too small for write */
    gm_result_size_t write_result = gm_cbor_write_uint(buffer, 0, 42);
    assert(!write_result.ok);
    assert_error_code(write_result.u.err, 6002); /* GM_ERROR_CBOR_BUFFER_TOO_SMALL */
    
    /* Test NULL buffer */
    write_result = gm_cbor_write_uint(NULL, BUFFER_SIZE, 42);
    assert(!write_result.ok);
    assert_error_code(write_result.u.err, 6003); /* GM_ERROR_CBOR_INVALID_DATA */
    
    /* Test type mismatch on read */
    buffer[0] = CBOR_TYPE_TEXT | 5; /* Text string, not uint */
    offset = 0;
    gm_result_uint64_t read_result = gm_cbor_read_uint(buffer, &offset, BUFFER_SIZE);
    assert(!read_result.ok);
    assert_error_code(read_result.u.err, 6001); /* GM_ERROR_CBOR_TYPE_MISMATCH */
    
    printf("OK\n");
}

/* Test byte string encoding and decoding */
static void test_cbor_bytes(void) {
    printf("test_cbor_bytes... ");
    
    uint8_t buffer[BUFFER_SIZE];
    uint8_t data[256];
    uint8_t read_data[256];
    size_t offset;
    
    /* Initialize test data */
    for (size_t i = 0; i < sizeof(data); i++) {
        data[i] = (uint8_t)i;
    }
    
    /* Test various lengths */
    size_t test_lengths[] = {0, 1, 23, 24, 255, 256};
    
    for (size_t i = 0; i < sizeof(test_lengths) / sizeof(test_lengths[0]); i++) {
        size_t len = test_lengths[i];
        if (len > sizeof(data)) continue;
        
        /* Write bytes */
        gm_result_size_t write_result = gm_cbor_write_bytes(buffer, BUFFER_SIZE, data, len);
        assert(write_result.ok);
        
        /* Read bytes back */
        memset(read_data, 0xFF, sizeof(read_data));
        offset = 0;
        gm_result_void_t read_result = gm_cbor_read_bytes(buffer, &offset, BUFFER_SIZE, 
                                                          read_data, len);
        assert(read_result.ok);
        assert(memcmp(data, read_data, len) == 0);
    }
    
    /* Test length mismatch */
    gm_result_size_t write_result = gm_cbor_write_bytes(buffer, BUFFER_SIZE, data, 10);
    assert(write_result.ok);
    
    offset = 0;
    gm_result_void_t read_result = gm_cbor_read_bytes(buffer, &offset, BUFFER_SIZE, 
                                                      read_data, 11); /* Wrong length */
    assert(!read_result.ok);
    assert_error_code(read_result.u.err, 6003); /* GM_ERROR_CBOR_INVALID_DATA */
    
    /* Test buffer overflow protection */
    /* Set up a byte string that would overflow when read */
    buffer[BUFFER_SIZE - 5] = CBOR_TYPE_BYTES | 10; /* 10 bytes needed, but only 4 available */
    offset = BUFFER_SIZE - 5; /* Not enough space */
    read_result = gm_cbor_read_bytes(buffer, &offset, BUFFER_SIZE, read_data, 10);
    assert(!read_result.ok);
    assert_error_code(read_result.u.err, 6002); /* GM_ERROR_CBOR_BUFFER_TOO_SMALL */
    
    printf("OK\n");
}

/* Test text string encoding and decoding */
static void test_cbor_text(void) {
    printf("test_cbor_text... ");
    
    uint8_t buffer[BUFFER_SIZE];
    char text[256];
    size_t offset;
    
    /* Test strings */
    const char *test_strings[] = {
        "",                      /* Empty string */
        "Hello",                 /* Short string */
        "This is a test string", /* Medium string */
        ("Lorem ipsum dolor sit amet, consectetur adipiscing elit. "
        "Sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.") /* Long string */
    };
    
    for (size_t i = 0; i < sizeof(test_strings) / sizeof(test_strings[0]); i++) {
        const char *str = test_strings[i];
        
        /* Write text */
        gm_result_size_t write_result = gm_cbor_write_text(buffer, BUFFER_SIZE, str);
        assert(write_result.ok);
        assert(write_result.u.val == strlen(str) + (strlen(str) < 24 ? 1 : 
                                                    strlen(str) <= 255 ? 2 : 3));
        
        /* Read text back */
        memset(text, 0xFF, sizeof(text));
        offset = 0;
        gm_result_void_t read_result = gm_cbor_read_text(buffer, &offset, BUFFER_SIZE, 
                                                         text, sizeof(text));
        assert(read_result.ok);
        assert(strcmp(str, text) == 0);
        assert(offset == write_result.u.val);
    }
    
    /* Test text buffer too small */
    gm_result_size_t write_result = gm_cbor_write_text(buffer, BUFFER_SIZE, "Hello World");
    assert(write_result.ok);
    
    offset = 0;
    gm_result_void_t read_result = gm_cbor_read_text(buffer, &offset, BUFFER_SIZE, 
                                                     text, 5); /* Too small */
    assert(!read_result.ok);
    assert_error_code(read_result.u.err, 6004); /* GM_ERROR_CBOR_OVERFLOW */
    
    /* Test UTF-8 text */
    const char *utf8_text = "Hello 世界 🌍";
    write_result = gm_cbor_write_text(buffer, BUFFER_SIZE, utf8_text);
    assert(write_result.ok);
    
    offset = 0;
    read_result = gm_cbor_read_text(buffer, &offset, BUFFER_SIZE, text, sizeof(text));
    assert(read_result.ok);
    assert(strcmp(utf8_text, text) == 0);
    
    printf("OK\n");
}

/* Test edge cases and error handling */
static void test_cbor_edge_cases(void) {
    printf("test_cbor_edge_cases... ");
    
    uint8_t buffer[BUFFER_SIZE];
    size_t offset;
    
    /* Test NULL pointer handling */
    gm_result_uint64_t uint_result = gm_cbor_read_uint(NULL, &offset, BUFFER_SIZE);
    assert(!uint_result.ok);
    assert_error_code(uint_result.u.err, 6003); /* GM_ERROR_CBOR_INVALID_DATA */
    
    uint_result = gm_cbor_read_uint(buffer, NULL, BUFFER_SIZE);
    assert(!uint_result.ok);
    assert_error_code(uint_result.u.err, 6003); /* GM_ERROR_CBOR_INVALID_DATA */
    
    /* Test invalid additional info */
    buffer[0] = CBOR_TYPE_UNSIGNED | 0x1C; /* Reserved value */
    offset = 0;
    uint_result = gm_cbor_read_uint(buffer, &offset, BUFFER_SIZE);
    assert(!uint_result.ok);
    assert_error_code(uint_result.u.err, 6003); /* GM_ERROR_CBOR_INVALID_DATA */
    
    /* Test reading past buffer end */
    buffer[BUFFER_SIZE - 2] = CBOR_TYPE_UNSIGNED | CBOR_UINT32_FOLLOWS;
    offset = BUFFER_SIZE - 2; /* Not enough space for uint32 */
    uint_result = gm_cbor_read_uint(buffer, &offset, BUFFER_SIZE);
    assert(!uint_result.ok);
    assert_error_code(uint_result.u.err, 6002); /* GM_ERROR_CBOR_BUFFER_TOO_SMALL */
    
    /* Test empty text buffer */
    char text[1];
    offset = 0;
    gm_result_void_t text_result = gm_cbor_read_text(buffer, &offset, BUFFER_SIZE, 
                                                     text, 0);
    assert(!text_result.ok);
    assert_error_code(text_result.u.err, 6003); /* GM_ERROR_CBOR_INVALID_DATA */
    
    printf("OK\n");
}

/* Test real-world CBOR sequences */
static void test_cbor_sequences(void) {
    printf("test_cbor_sequences... ");
    
    uint8_t buffer[BUFFER_SIZE];
    size_t write_offset = 0;
    size_t read_offset = 0;
    
    /* Write a sequence of values */
    uint64_t uint_val = 42;
    const uint8_t bytes_val[] = {0xDE, 0xAD, 0xBE, 0xEF};
    const char *text_val = "Hello CBOR";
    
    /* Write uint */
    gm_result_size_t write_result = gm_cbor_write_uint(buffer + write_offset, 
                                                       BUFFER_SIZE - write_offset, uint_val);
    assert(write_result.ok);
    write_offset += write_result.u.val;
    
    /* Write bytes */
    write_result = gm_cbor_write_bytes(buffer + write_offset, 
                                      BUFFER_SIZE - write_offset, 
                                      bytes_val, sizeof(bytes_val));
    assert(write_result.ok);
    write_offset += write_result.u.val;
    
    /* Write text */
    write_result = gm_cbor_write_text(buffer + write_offset, 
                                     BUFFER_SIZE - write_offset, text_val);
    assert(write_result.ok);
    write_offset += write_result.u.val;
    
    /* Read back in same order */
    gm_result_uint64_t uint_result = gm_cbor_read_uint(buffer, &read_offset, write_offset);
    assert(uint_result.ok);
    assert(uint_result.u.val == uint_val);
    
    uint8_t read_bytes[4];
    gm_result_void_t bytes_result = gm_cbor_read_bytes(buffer, &read_offset, write_offset,
                                                       read_bytes, sizeof(bytes_val));
    assert(bytes_result.ok);
    assert(memcmp(bytes_val, read_bytes, sizeof(bytes_val)) == 0);
    
    char read_text[64];
    gm_result_void_t text_result = gm_cbor_read_text(buffer, &read_offset, write_offset,
                                                     read_text, sizeof(read_text));
    assert(text_result.ok);
    assert(strcmp(text_val, read_text) == 0);
    
    /* Verify we consumed entire buffer */
    assert(read_offset == write_offset);
    
    printf("OK\n");
}

/* Test bounds checking thoroughly */
static void test_cbor_bounds_checking(void) {
    printf("test_cbor_bounds_checking... ");
    
    uint8_t small_buffer[10];
    size_t offset;
    
    /* Test writing to small buffers */
    struct {
        uint64_t value;
        size_t buf_size;
        bool should_succeed;
    } write_tests[] = {
        {0, 1, true},      /* Exact fit */
        {0, 0, false},     /* Too small */
        {24, 2, true},     /* Exact fit for uint8 */
        {24, 1, false},    /* Too small for uint8 */
        {256, 3, true},    /* Exact fit for uint16 */
        {256, 2, false},   /* Too small for uint16 */
    };
    
    for (size_t i = 0; i < sizeof(write_tests) / sizeof(write_tests[0]); i++) {
        gm_result_size_t result = gm_cbor_write_uint(small_buffer, 
                                                     write_tests[i].buf_size,
                                                     write_tests[i].value);
        assert(result.ok == write_tests[i].should_succeed);
        if (!result.ok) {
            assert(result.u.err->code == 6002); /* GM_ERROR_CBOR_BUFFER_TOO_SMALL */
            gm_error_free(result.u.err);
        }
    }
    
    /* Test reading from truncated buffers */
    small_buffer[0] = CBOR_TYPE_UNSIGNED | CBOR_UINT64_FOLLOWS;
    for (size_t max_size = 1; max_size < 9; max_size++) {
        offset = 0;
        gm_result_uint64_t result = gm_cbor_read_uint(small_buffer, &offset, max_size);
        assert(!result.ok);
        assert(result.u.err->code == 6002); /* GM_ERROR_CBOR_BUFFER_TOO_SMALL */
        gm_error_free(result.u.err);
    }
    
    /* Test text overflow */
    const char *long_text = "This is a very long string";
    gm_result_size_t write_result = gm_cbor_write_text(small_buffer, 5, long_text);
    assert(!write_result.ok);
    assert_error_code(write_result.u.err, 6002); /* GM_ERROR_CBOR_BUFFER_TOO_SMALL */
    
    printf("OK\n");
}

int main(void) {
    printf("Running CBOR tests...\n");
    
    test_cbor_uint();
    test_cbor_bytes();
    test_cbor_text();
    test_cbor_edge_cases();
    test_cbor_sequences();
    test_cbor_bounds_checking();
    
    printf("All tests passed!\n");
    return 0;
}
