/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "gitmind.h"
#include "gitmind/attribution.h"

void test_attribution_defaults() {
    printf("Testing attribution defaults...\n");
    
    gm_attribution_t attr;
    
    /* Test human default */
    gm_attribution_set_default(&attr, GM_SOURCE_HUMAN);
    assert(attr.source_type == GM_SOURCE_HUMAN);
    assert(strcmp(attr.author, "user@local") == 0);
    
    /* Test Claude default */
    gm_attribution_set_default(&attr, GM_SOURCE_AI_CLAUDE);
    assert(attr.source_type == GM_SOURCE_AI_CLAUDE);
    assert(strcmp(attr.author, "claude@anthropic") == 0);
    
    printf("✓ Attribution defaults work\n");
}

void test_filters() {
    printf("Testing filters...\n");
    
    gm_filter_t filter;
    gm_edge_attributed_t edge = {0};
    
    /* Set up test edge */
    edge.attribution.source_type = GM_SOURCE_HUMAN;
    edge.confidence = 0x3C00;  /* 1.0 in half-float */
    
    /* Test human-only filter */
    gm_filter_init_human_only(&filter);
    assert(gm_filter_match(&filter, &edge) == 1);
    
    /* Change to AI edge */
    edge.attribution.source_type = GM_SOURCE_AI_CLAUDE;
    assert(gm_filter_match(&filter, &edge) == 0);
    
    /* Test AI insights filter */
    gm_filter_init_ai_insights(&filter, 0.8);
    assert(gm_filter_match(&filter, &edge) == 1);
    
    /* Lower confidence - should not match because below threshold */
    edge.confidence = 0x2C00;  /* ~0.375 in half-float */
    assert(gm_filter_match(&filter, &edge) == 0);
    
    printf("✓ Filters work correctly\n");
}

void test_cbor_round_trip() {
    printf("Testing CBOR round-trip (encode + decode)...\n");
    
    gm_edge_attributed_t edge = {0};
    gm_edge_attributed_t decoded = {0};
    uint8_t buffer[512];
    size_t len = sizeof(buffer);
    
    /* Set up test edge with all fields */
    memset(edge.src_sha, 0xAA, 20);
    memset(edge.tgt_sha, 0xBB, 20);
    edge.rel_type = GM_REL_IMPLEMENTS;
    edge.confidence = 0x3C00;  /* 1.0 */
    edge.timestamp = 1234567890;
    strcpy(edge.src_path, "src/main.c");
    strcpy(edge.tgt_path, "docs/design.md");
    strcpy(edge.ulid, "01234567890123456789012345");
    
    edge.attribution.source_type = GM_SOURCE_AI_CLAUDE;
    strcpy(edge.attribution.author, "claude@anthropic");
    strcpy(edge.attribution.session_id, "conv_123");
    edge.attribution.flags = GM_ATTR_REVIEWED | GM_ATTR_ACCEPTED;
    edge.lane = GM_LANE_ARCHITECTURE;
    
    /* Encode */
    int ret = gm_edge_attributed_encode_cbor(&edge, buffer, &len);
    assert(ret == 0);
    assert(len > 0);
    assert(len < sizeof(buffer));
    
    /* Decode */
    ret = gm_edge_attributed_decode_cbor(buffer, len, &decoded);
    assert(ret == 0);
    
    /* Verify all fields match */
    assert(memcmp(decoded.src_sha, edge.src_sha, 20) == 0);
    assert(memcmp(decoded.tgt_sha, edge.tgt_sha, 20) == 0);
    assert(decoded.rel_type == edge.rel_type);
    assert(decoded.confidence == edge.confidence);
    assert(decoded.timestamp == edge.timestamp);
    assert(strcmp(decoded.src_path, edge.src_path) == 0);
    assert(strcmp(decoded.tgt_path, edge.tgt_path) == 0);
    assert(strcmp(decoded.ulid, edge.ulid) == 0);
    
    assert(decoded.attribution.source_type == edge.attribution.source_type);
    assert(strcmp(decoded.attribution.author, edge.attribution.author) == 0);
    assert(strcmp(decoded.attribution.session_id, edge.attribution.session_id) == 0);
    assert(decoded.attribution.flags == edge.attribution.flags);
    assert(decoded.lane == edge.lane);
    
    printf("✓ CBOR round-trip works (encoded %zu bytes)\n", len);
}

void test_cbor_edge_cases() {
    printf("Testing CBOR edge cases...\n");
    
    gm_edge_attributed_t edge = {0};
    gm_edge_attributed_t decoded = {0};
    uint8_t buffer[512];
    size_t len;
    
    /* Test with empty strings */
    memset(edge.src_sha, 0x11, 20);
    memset(edge.tgt_sha, 0x22, 20);
    edge.rel_type = GM_REL_REFERENCES;
    edge.confidence = 0x3666;  /* 0.85 */
    edge.timestamp = 9999999999;
    strcpy(edge.src_path, "a");  /* Short path */
    strcpy(edge.tgt_path, "");   /* Empty path */
    strcpy(edge.ulid, "00000000000000000000000000");
    
    edge.attribution.source_type = GM_SOURCE_HUMAN;
    edge.attribution.author[0] = '\0';  /* Empty author */
    edge.attribution.session_id[0] = '\0';  /* Empty session */
    edge.attribution.flags = 0;
    edge.lane = GM_LANE_DEFAULT;
    
    len = sizeof(buffer);
    int ret = gm_edge_attributed_encode_cbor(&edge, buffer, &len);
    assert(ret == 0);
    
    ret = gm_edge_attributed_decode_cbor(buffer, len, &decoded);
    assert(ret == 0);
    
    /* Verify empty strings handled correctly */
    assert(strlen(decoded.tgt_path) == 0);
    assert(strlen(decoded.attribution.author) == 0);
    assert(strlen(decoded.attribution.session_id) == 0);
    
    /* Test invalid CBOR */
    uint8_t bad_cbor[] = {0xFF, 0xFF, 0xFF};
    ret = gm_edge_attributed_decode_cbor(bad_cbor, sizeof(bad_cbor), &decoded);
    assert(ret != 0);
    
    /* Test truncated CBOR */
    ret = gm_edge_attributed_decode_cbor(buffer, 10, &decoded);  /* Too short */
    assert(ret != 0);
    
    printf("✓ CBOR edge cases handled correctly\n");
}

int main() {
    printf("=== Attribution System Tests ===\n");
    
    test_attribution_defaults();
    test_filters();
    test_cbor_round_trip();
    test_cbor_edge_cases();
    
    printf("\nAll tests passed! ✅\n");
    return 0;
}