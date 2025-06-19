/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include <stdio.h>
#include <assert.h>
#include <string.h>
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
    
    /* Lower confidence */
    edge.confidence = 0x3400;  /* ~0.5 in half-float */
    assert(gm_filter_match(&filter, &edge) == 0);
    
    printf("✓ Filters work correctly\n");
}

void test_cbor_encoding() {
    printf("Testing CBOR encoding...\n");
    
    gm_edge_attributed_t edge = {0};
    uint8_t buffer[512];
    size_t len = sizeof(buffer);
    
    /* Set up test edge */
    memset(edge.src_sha, 0xAA, 20);
    memset(edge.tgt_sha, 0xBB, 20);
    edge.rel_type = GM_REL_IMPLEMENTS;
    edge.confidence = 0x3C00;
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
    
    printf("✓ CBOR encoding works (encoded %zu bytes)\n", len);
}

int main() {
    printf("=== Attribution System Tests ===\n");
    
    test_attribution_defaults();
    test_filters();
    test_cbor_encoding();
    
    printf("\nAll tests passed! ✅\n");
    return 0;
}