/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/error.h"
#include "gitmind/result.h"
#include "gitmind/types/id.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* Test ID creation from string */
static void test_id_from_string(void) {
    gm_result_id_t result1 = gm_id_from_string("hello");
    assert(GM_IS_OK(result1));
    gm_id_t id1 = GM_UNWRAP(result1);

    gm_result_id_t result2 = gm_id_from_string("hello");
    assert(GM_IS_OK(result2));
    gm_id_t id2 = GM_UNWRAP(result2);

    gm_result_id_t result3 = gm_id_from_string("world");
    assert(GM_IS_OK(result3));
    gm_id_t id3 = GM_UNWRAP(result3);

    /* Same input should produce same ID */
    assert(gm_id_equal(id1, id2));

    /* Different input should produce different ID */
    assert(!gm_id_equal(id1, id3));

    /* Test error case - nullptr input */
    gm_result_id_t err_result = gm_id_from_string(nullptr);
    assert(GM_IS_ERR(err_result));
    gm_error_free(GM_UNWRAP_ERR(err_result));

    printf("✓ test_id_from_string\n");
}

/* Test ID to/from hex */
static void test_id_hex_conversion(void) {
    gm_result_id_t id_result = gm_id_from_string("test");
    assert(GM_IS_OK(id_result));
    gm_id_t test_id = GM_UNWRAP(id_result);

    char hex[GM_ID_HEX_SIZE];
    gm_result_void_t hex_result = gm_id_to_hex(test_id, hex, sizeof(hex));
    assert(GM_IS_OK(hex_result));
    assert(strlen(hex) == GM_ID_HEX_CHARS); /* SHA-256 hex representation */

    /* Test buffer too small */
    char small_buf[10];
    gm_result_void_t small_result =
        gm_id_to_hex(test_id, small_buf, sizeof(small_buf));
    assert(GM_IS_ERR(small_result));
    assert(GM_UNWRAP_ERR(small_result)->code == GM_ERR_BUFFER_TOO_SMALL);
    gm_error_free(GM_UNWRAP_ERR(small_result));

    /* Parse back */
    gm_result_id_t result = gm_id_from_hex(hex);
    assert(GM_IS_OK(result));

    gm_id_t parsed = GM_UNWRAP(result);
    assert(gm_id_equal(test_id, parsed));

    printf("✓ test_id_hex_conversion\n");
}

/* Test invalid hex parsing */
static void test_id_hex_invalid(void) {
    /* Too short */
    gm_result_id_t result = gm_id_from_hex("abc");
    assert(GM_IS_ERR(result));
    gm_error_free(GM_UNWRAP_ERR(result));

    /* Invalid characters */
    result = gm_id_from_hex(
        "zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz");
    assert(GM_IS_ERR(result));
    gm_error_free(GM_UNWRAP_ERR(result));

    /* nullptr */
    result = gm_id_from_hex(nullptr);
    assert(GM_IS_ERR(result));
    gm_error_free(GM_UNWRAP_ERR(result));

    printf("✓ test_id_hex_invalid\n");
}

/* Test strongly typed IDs */
static void test_typed_ids(void) {
    gm_result_id_t node1_result = gm_id_from_string("node1");
    assert(GM_IS_OK(node1_result));
    gm_node_id_t node1 = {.base = GM_UNWRAP(node1_result)};

    gm_result_id_t node2_result = gm_id_from_string("node2");
    assert(GM_IS_OK(node2_result));
    gm_node_id_t node2 = {.base = GM_UNWRAP(node2_result)};

    gm_result_id_t edge1_result = gm_id_from_string("edge1");
    assert(GM_IS_OK(edge1_result));
    gm_edge_id_t edge1 = {.base = GM_UNWRAP(edge1_result)};

    gm_result_id_t edge2_result = gm_id_from_string("edge2");
    assert(GM_IS_OK(edge2_result));
    gm_edge_id_t edge2 = {.base = GM_UNWRAP(edge2_result)};

    /* Can compare same types */
    assert(!gm_node_id_equal(node1, node2));
    assert(!gm_edge_id_equal(edge1, edge2));

    /* Cannot compare different types (won't compile) */
    /* assert(gm_node_id_equal(node1, edge1)); // COMPILE ERROR! */

    /* Can convert to hex */
    char hex[GM_ID_HEX_SIZE];
    gm_node_id_to_hex(node1, hex);
    assert(strlen(hex) == GM_ID_HEX_CHARS);

    /* Edge IDs also convert to hex */
    gm_edge_id_to_hex(edge1, hex);
    assert(strlen(hex) == GM_ID_HEX_CHARS);

    printf("✓ test_typed_ids\n");
}

/* Test ID generation */
static void test_id_generate(void) {
    gm_result_id_t result1 = gm_id_generate();
    assert(GM_IS_OK(result1));
    gm_id_t id1 = GM_UNWRAP(result1);

    gm_result_id_t result2 = gm_id_generate();
    assert(GM_IS_OK(result2));
    gm_id_t id2 = GM_UNWRAP(result2);

    /* Generated IDs should be different */
    assert(!gm_id_equal(id1, id2));

    printf("✓ test_id_generate\n");
}

/* Test session ID */
static void test_session_id(void) {
    gm_result_session_id_t result1 = gm_session_id_new();
    assert(GM_IS_OK(result1));
    gm_session_id_t sid1 = GM_UNWRAP(result1);

    gm_result_session_id_t result2 = gm_session_id_new();
    assert(GM_IS_OK(result2));
    gm_session_id_t sid2 = GM_UNWRAP(result2);

    /* Session IDs should be unique */
    assert(!gm_session_id_equal(sid1, sid2));

    printf("✓ test_session_id\n");
}

/* Test ID creation from data */
static void test_id_from_data(void) {
    const uint8_t data1[] = {0x01, 0x02, 0x03, 0x04};
    const uint8_t data2[] = {0x01, 0x02, 0x03, 0x04};
    const uint8_t data3[] = {0x04, 0x03, 0x02, 0x01};

    gm_result_id_t result1 = gm_id_from_data(data1, sizeof(data1));
    assert(GM_IS_OK(result1));
    gm_id_t id1 = GM_UNWRAP(result1);

    gm_result_id_t result2 = gm_id_from_data(data2, sizeof(data2));
    assert(GM_IS_OK(result2));
    gm_id_t id2 = GM_UNWRAP(result2);

    gm_result_id_t result3 = gm_id_from_data(data3, sizeof(data3));
    assert(GM_IS_OK(result3));
    gm_id_t id3 = GM_UNWRAP(result3);

    /* Same data should produce same ID */
    assert(gm_id_equal(id1, id2));

    /* Different data should produce different ID */
    assert(!gm_id_equal(id1, id3));

    /* Test error case - nullptr data */
    gm_result_id_t err_result = gm_id_from_data(nullptr, 10);
    assert(GM_IS_ERR(err_result));
    gm_error_free(GM_UNWRAP_ERR(err_result));

    /* Test zero length - should succeed (SHA256 of empty data is valid) */
    gm_result_id_t zero_result = gm_id_from_data(data1, 0);
    assert(GM_IS_OK(zero_result));
    /* SHA256 of empty string should be
     * e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855 */

    printf("✓ test_id_from_data\n");
}

/* Test improved hash function */
static void test_id_hash(void) {
    /* Create IDs that differ only in the last bytes */
    gm_id_t id1, id2;
    memset(id1.bytes, 0xAA, GM_ID_SIZE);
    memset(id2.bytes, 0xAA, GM_ID_SIZE);
    id2.bytes[GM_ID_SIZE - 1] = 0xBB; /* Only last byte different */

    gm_result_u32_t hash1_result = gm_id_hash(id1);
    assert(GM_IS_OK(hash1_result));
    uint32_t hash1 = GM_UNWRAP(hash1_result);

    gm_result_u32_t hash2_result = gm_id_hash(id2);
    assert(GM_IS_OK(hash2_result));
    uint32_t hash2 = GM_UNWRAP(hash2_result);

    /* Hashes should be different (SipHash uses all bytes) */
    assert(hash1 != hash2);

    /* Create IDs that differ only in the first bytes */
    gm_id_t id3, id4;
    memset(id3.bytes, 0xCC, GM_ID_SIZE);
    memset(id4.bytes, 0xCC, GM_ID_SIZE);
    id4.bytes[0] = 0xDD; /* Only first byte different */

    gm_result_u32_t hash3_result = gm_id_hash(id3);
    assert(GM_IS_OK(hash3_result));
    uint32_t hash3 = GM_UNWRAP(hash3_result);

    gm_result_u32_t hash4_result = gm_id_hash(id4);
    assert(GM_IS_OK(hash4_result));
    uint32_t hash4 = GM_UNWRAP(hash4_result);

    /* Hashes should be different */
    assert(hash3 != hash4);

    /* Test that same ID produces same hash */
    gm_result_u32_t rehash1 = gm_id_hash(id1);
    assert(GM_IS_OK(rehash1) && GM_UNWRAP(rehash1) == hash1);

    gm_result_u32_t rehash2 = gm_id_hash(id2);
    assert(GM_IS_OK(rehash2) && GM_UNWRAP(rehash2) == hash2);

    printf("✓ test_id_hash\n");
}

int main(void) {
    printf("Running ID type tests...\n\n");

    test_id_from_string();
    test_id_from_data();
    test_id_hex_conversion();
    test_id_hex_invalid();
    test_typed_ids();
    test_id_generate();
    test_session_id();
    test_id_hash();

    printf("\n✅ All ID tests passed!\n");
    return 0;
}