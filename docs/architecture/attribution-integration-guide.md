<!-- SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 -->
<!-- © 2025 J. Kirby Ross / Neuroglyph Collective -->
---
title: Attribution System Integration Guide
description: How to adopt attribution lanes and metadata in existing modules and CLI commands.
audience: [developers]
domain: [architecture]
tags: [attribution, integration, cli]
status: stable
last_updated: 2025-09-15
---

# Attribution System Integration Guide

Table of Contents

- [Quick Start](#quick-start)
- [For CLI Commands](#for-cli-commands)
- [For Filtering](#for-filtering)

This guide explains how to integrate the attribution system into existing git-mind components.

## Quick Start

### For CLI Commands

```c
#include "gitmind/attribution.h"

// In your command handler:
gm_edge_attributed_t edge = {0};
gm_attribution_t attr;

// Get attribution from environment
gm_attribution_from_env(&attr);
edge.attribution = attr;

// Set edge data as before
edge.src_sha = ...;
edge.confidence = 0x3C00;  // 1.0 for human
```

### For Filtering

```c
// Show only human edges
gm_filter_t filter;
gm_filter_init_human_only(&filter);

// Check each edge
if (gm_filter_match(&filter, &edge)) {
    // Display edge
}
```

## Integration Checklist

### 1. Update `git mind link` Command

__File__: `src/cli/link.c`

```c
// Add to link_cmd():
gm_edge_attributed_t edge = {0};

// Get attribution
gm_attribution_from_env(&edge.attribution);

// If AI is creating edge, get confidence from args
if (edge.attribution.source_type != GM_SOURCE_HUMAN) {
    edge.confidence = parse_confidence(args);
} else {
    edge.confidence = 0x3C00;  // 1.0 for humans
}

// Encode with attribution
gm_edge_attributed_encode_cbor(&edge, buffer, &len);
```

### 2. Update `git mind list` Command

__File__: `src/cli/list.c`

Add new options:

```c
--source <human|ai|all>     Filter by source
--min-confidence <float>    Minimum confidence
--lane <name>              Filter by lane
--pending                  Show pending AI suggestions
--show-attribution         Display source info
```

Example implementation:

```c
// Parse filter options
gm_filter_t filter;
if (strcmp(source_arg, "human") == 0) {
    gm_filter_init_human_only(&filter);
} else if (strcmp(source_arg, "ai") == 0) {
    gm_filter_init_ai_insights(&filter, min_conf);
} else {
    gm_filter_init_default(&filter);
}

// Apply filter while listing
gm_edge_attributed_t edge;
while (read_next_edge(&edge)) {
    if (gm_filter_match(&filter, &edge)) {
        display_edge(&edge);
    }
}
```

### 3. Update Journal Reader

__File__: `src/journal/reader.c`

```c
// Update edge callback to handle attributed edges
int process_edge(const uint8_t *cbor_data, size_t len, void *userdata) {
    gm_edge_attributed_t edge;
    
    // Try new format first
    if (gm_edge_attributed_decode_cbor(cbor_data, len, &edge) == 0) {
        // Process attributed edge
    } else {
        // Fall back to legacy format
        gm_edge_t legacy_edge;
        if (gm_edge_decode_cbor(cbor_data, len, &legacy_edge) == 0) {
            // Convert to attributed with defaults
            memcpy(&edge, &legacy_edge, sizeof(gm_edge_t));
            gm_attribution_set_default(&edge.attribution, GM_SOURCE_HUMAN);
            edge.lane = GM_LANE_DEFAULT;
        }
    }
}
```

### 4. Update Journal Writer

__File__: `src/journal/writer.c`

```c
// Use attributed edges
int gm_journal_append_attributed(gm_context_t *ctx, 
                                const gm_edge_attributed_t *edges, 
                                size_t n_edges) {
    for (size_t i = 0; i < n_edges; i++) {
        uint8_t buffer[512];
        size_t len = sizeof(buffer);
        
        gm_edge_attributed_encode_cbor(&edges[i], buffer, &len);
        // Write to journal as before
    }
}
```

### 5. Add Review Command (New)

__File__: `src/cli/review.c`

```c
int review_cmd(int argc, char **argv) {
    // List pending AI suggestions
    gm_filter_t filter;
    gm_filter_init_default(&filter);
    filter.flags_required = GM_ATTR_PENDING;
    
    // Show each pending edge
    // User can accept/reject/modify
}
```

## Environment Variables

### For Humans (Default)

```bash
# Nothing needed - defaults to human
git mind link src/main.c docs/api.md --type implements
```

### For AI Systems

```bash
# Claude via MCP
export GIT_MIND_SOURCE=claude
export GIT_MIND_AUTHOR=claude@anthropic
export GIT_MIND_SESSION=conv_12345

# GPT-4
export GIT_MIND_SOURCE=gpt
export GIT_MIND_AUTHOR=gpt4@openai
```

### For System/Hooks

```bash
# AUGMENTS hook
export GIT_MIND_SOURCE=system
export GIT_MIND_AUTHOR=augments@git-mind
```

## Display Formatting

### Basic Display

```
src/main.c ──implements──> docs/design.md
```

### With Attribution

```
src/main.c ──implements──> docs/design.md [human: user@example.com]
src/parser.c ──depends_on──> lib/ast.c [ai: claude@anthropic, conf: 0.85]
src/test.c ──tests──> src/main.c [consensus: human+ai]
```

### Pending Review Display

```
PENDING: src/auth.c ──likely_depends_on──> config/oauth.json
         Source: claude@anthropic
         Confidence: 0.82
         Reason: Changed together in 89% of commits
         [A]ccept [R]eject [M]odify [S]kip
```

## Testing

### Unit Tests

```c
void test_attribution_filtering() {
    gm_edge_attributed_t human_edge = {0};
    gm_edge_attributed_t ai_edge = {0};
    
    human_edge.attribution.source_type = GM_SOURCE_HUMAN;
    ai_edge.attribution.source_type = GM_SOURCE_AI_CLAUDE;
    ai_edge.confidence = 0x3666;  // 0.85
    
    gm_filter_t filter;
    gm_filter_init_human_only(&filter);
    
    assert(gm_filter_match(&filter, &human_edge) == 1);
    assert(gm_filter_match(&filter, &ai_edge) == 0);
}
```

### Integration Tests

```bash
# Test human edges
git mind link src/a.c src/b.c --type depends_on
git mind list --source human
# Should show the edge

# Test AI edges
export GIT_MIND_SOURCE=claude
git mind link src/c.c src/d.c --type references --confidence 0.7
git mind list --source ai --min-confidence 0.6
# Should show the edge
```

## Migration Path

### Phase 1: Soft Launch

- Attribution system available but optional
- Legacy edges get default attribution
- New edges use attribution if environment set

### Phase 2: Default On

- All new edges get attribution
- Add migration command for old edges
- Web UI shows attribution

### Phase 3: Full Integration

- MCP tools create attributed edges
- Review workflow enabled
- Analytics on human vs AI contributions

## Common Patterns

### AI Batch Analysis

```python
# Claude analyzing codebase
for file_pair in analyze_coupling():
    edge = {
        'source': file_pair[0],
        'target': file_pair[1],
        'type': 'coupled_with',
        'confidence': calculate_confidence(file_pair),
        'attribution': {
            'source_type': 'claude',
            'session_id': session_id,
            'flags': 'pending'
        }
    }
    create_edge(edge)
```

### Human Review Session

```bash
# Review all pending
git mind review --pending

# Accept high-confidence edges
git mind review --pending --min-confidence 0.8 --auto-accept

# Reject specific pattern
git mind review --pending --type coupled_with --reject-all
```

### Consensus Building

```sql
-- Find where human and AI agree
SELECT * FROM edges e1
JOIN edges e2 ON e1.src = e2.src AND e1.tgt = e2.tgt
WHERE e1.source_type = 'human' 
  AND e2.source_type = 'ai'
  AND e1.rel_type = e2.rel_type
```

## Troubleshooting

### Edge Not Showing Up

Check filters:

```bash
git mind list --source all --no-filter
```

### Attribution Not Set

Check environment:

```bash
env | grep GIT_MIND
```

### Legacy Edges

Migrate them:

```bash
git mind migrate --add-attribution
```

---

_With attribution integrated, git-mind becomes a platform for human-AI collaboration, not just a tool for storing connections._
