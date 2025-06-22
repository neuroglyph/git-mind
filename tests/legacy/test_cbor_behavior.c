/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

/*
 * BEHAVIORAL TESTS FOR CBOR MODULE
 * 
 * These tests ensure CBOR encoding/decoding behaves correctly.
 * We test WHAT it does, not HOW it does it.
 */

#include "gitmind.h"
#include "gitmind/cbor_common.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>

/* Test CBOR encoding produces valid output */
void test_cbor_encode_valid_edge(void) {
    gm_edge_t edge = {0};
    
    /* Set up test edge */
    memset(edge.src_sha, 0xAA, GM_SHA1_SIZE);
    memset(edge.tgt_sha, 0xBB, GM_SHA1_SIZE);
    edge.rel_type = GM_REL_DEPENDS_ON;
    edge.confidence = 0x3C00;  /* 1.0 in IEEE half-float */
    edge.timestamp = 1234567890123ULL;
    strcpy(edge.src_path, "src/a.c");
    strcpy(edge.tgt_path, "src/b.c");
    
    /* Encode */
    uint8_t buffer[1024];
    size_t len = 0;
    int result = gm_edge_encode_cbor(&edge, buffer, &len);
    
    /* Verify success */
    assert(result == GM_OK);
    assert(len > 0);
    assert(len < sizeof(buffer));
    
    /* Verify CBOR structure (array with 7 elements) */
    assert((buffer[0] & 0xE0) == 0x80);  /* Array type */
    assert((buffer[0] & 0x1F) == 0x07);  /* 7 elements */
}

/* Test CBOR decoding handles valid input */
void test_cbor_decode_valid_data(void) {
    /* First create valid CBOR data */
    gm_edge_t original = {0};
    memset(original.src_sha, 0xAA, GM_SHA1_SIZE);
    memset(original.tgt_sha, 0xBB, GM_SHA1_SIZE);
    original.rel_type = GM_REL_IMPLEMENTS;
    original.confidence = 0x3800;  /* 0.5 in IEEE half-float */
    original.timestamp = 9876543210ULL;
    strcpy(original.src_path, "lib/foo.c");
    strcpy(original.tgt_path, "lib/bar.c");
    
    uint8_t buffer[1024];
    size_t len = 0;
    assert(gm_edge_encode_cbor(&original, buffer, &len) == GM_OK);
    
    /* Now decode it */
    gm_edge_t decoded = {0};
    int result = gm_edge_decode_cbor(buffer, len, &decoded);
    
    /* Verify success */
    assert(result == GM_OK);
    
    /* Verify data integrity */
    assert(memcmp(decoded.src_sha, original.src_sha, GM_SHA1_SIZE) == 0);
    assert(memcmp(decoded.tgt_sha, original.tgt_sha, GM_SHA1_SIZE) == 0);
    assert(decoded.rel_type == original.rel_type);
    assert(decoded.confidence == original.confidence);
    assert(decoded.timestamp == original.timestamp);
    assert(strcmp(decoded.src_path, original.src_path) == 0);
    assert(strcmp(decoded.tgt_path, original.tgt_path) == 0);
}

/* Test CBOR handles edge cases gracefully */
void test_cbor_edge_cases(void) {
    gm_edge_t edge = {0};
    uint8_t buffer[1024];
    size_t len;
    
    /* Test NULL parameters */
    assert(gm_edge_encode_cbor(NULL, buffer, &len) == GM_INVALID_ARG);
    assert(gm_edge_encode_cbor(&edge, NULL, &len) == GM_INVALID_ARG);
    assert(gm_edge_encode_cbor(&edge, buffer, NULL) == GM_INVALID_ARG);
    
    assert(gm_edge_decode_cbor(NULL, 10, &edge) == GM_INVALID_ARG);
    assert(gm_edge_decode_cbor(buffer, 10, NULL) == GM_INVALID_ARG);
    assert(gm_edge_decode_cbor(buffer, 0, &edge) == GM_INVALID_ARG);
    
    /* Test empty paths */
    memset(&edge, 0, sizeof(edge));
    edge.src_path[0] = '\0';
    edge.tgt_path[0] = '\0';
    assert(gm_edge_encode_cbor(&edge, buffer, &len) == GM_OK);
    assert(len > 0);
}

/* Test CBOR decode_ex provides consumed bytes */
void test_cbor_decode_ex_consumed(void) {
    /* Create valid CBOR data */
    gm_edge_t original = {0};
    memset(original.src_sha, 0xCC, GM_SHA1_SIZE);
    memset(original.tgt_sha, 0xDD, GM_SHA1_SIZE);
    original.rel_type = GM_REL_AUGMENTS;
    original.confidence = 0x3E00;  /* 1.5 in IEEE half-float */
    original.timestamp = 555666777ULL;
    strcpy(original.src_path, "test/x.c");
    strcpy(original.tgt_path, "test/y.c");
    
    uint8_t buffer[1024];
    size_t len = 0;
    assert(gm_edge_encode_cbor(&original, buffer, &len) == GM_OK);
    
    /* Add some junk after valid CBOR */
    memset(buffer + len, 0xFF, 100);
    
    /* Decode with consumed tracking */
    gm_edge_t decoded = {0};
    size_t consumed = 0;
    int result = gm_edge_decode_cbor_ex(buffer, len + 100, &decoded, &consumed);
    
    /* Verify success and correct consumed bytes */
    assert(result == GM_OK);
    assert(consumed == len);  /* Should only consume valid CBOR */
    assert(consumed < len + 100);  /* Should not consume junk */
    
    /* Verify decoded data */
    assert(memcmp(decoded.src_sha, original.src_sha, GM_SHA1_SIZE) == 0);
    assert(decoded.timestamp == original.timestamp);
}

/* Test CBOR handles maximum size paths */
void test_cbor_max_paths(void) {
    gm_edge_t edge = {0};
    
    /* Create maximum length paths */
    memset(edge.src_path, 'a', GM_PATH_MAX - 1);
    edge.src_path[GM_PATH_MAX - 1] = '\0';
    memset(edge.tgt_path, 'b', GM_PATH_MAX - 1);
    edge.tgt_path[GM_PATH_MAX - 1] = '\0';
    
    /* Should encode successfully */
    uint8_t buffer[2048];  /* Larger buffer for max paths */
    size_t len = 0;
    assert(gm_edge_encode_cbor(&edge, buffer, &len) == GM_OK);
    
    /* Should decode successfully */
    gm_edge_t decoded = {0};
    assert(gm_edge_decode_cbor(buffer, len, &decoded) == GM_OK);
    
    /* Verify paths preserved */
    assert(strcmp(decoded.src_path, edge.src_path) == 0);
    assert(strcmp(decoded.tgt_path, edge.tgt_path) == 0);
}

int main(void) {
    printf("🧪 Running CBOR behavioral tests...\n\n");
    
    printf("📦 Testing valid edge encoding...\n");
    test_cbor_encode_valid_edge();
    printf("✅ Encoding produces valid CBOR!\n\n");
    
    printf("📥 Testing valid data decoding...\n");
    test_cbor_decode_valid_data();
    printf("✅ Round-trip encoding/decoding works!\n\n");
    
    printf("🔧 Testing edge cases...\n");
    test_cbor_edge_cases();
    printf("✅ Edge cases handled gracefully!\n\n");
    
    printf("📏 Testing decode_ex consumed bytes...\n");
    test_cbor_decode_ex_consumed();
    printf("✅ Consumed bytes tracking works!\n\n");
    
    printf("📝 Testing maximum path lengths...\n");
    test_cbor_max_paths();
    printf("✅ Maximum paths handled correctly!\n\n");
    
    printf("🎉 ALL BEHAVIORAL TESTS PASSED!\n");
    printf("\nCBOR module behavior is verified and protected.\n");
    
    return 0;
}