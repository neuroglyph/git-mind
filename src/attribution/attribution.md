# Attribution System

The attribution system tracks who (human or AI) created each semantic edge, enabling collaborative knowledge building between humans and AI systems.

## Overview

Every edge in git-mind can now be attributed to its creator:
- Human users creating connections manually
- AI systems suggesting relationships
- System-generated edges (like AUGMENTS)

This enables:
- Filtering views by source
- Tracking AI suggestions vs human intent
- Building consensus when both agree
- Managing conflicts when they disagree

## Core Concepts

### Source Types

```c
typedef enum {
    GM_SOURCE_HUMAN        = 0,    /* Human-created edge */
    GM_SOURCE_AI_CLAUDE    = 1,    /* Claude AI via MCP */
    GM_SOURCE_AI_GPT      = 2,    /* GPT-4 or similar */
    GM_SOURCE_AI_OTHER    = 3,    /* Other AI systems */
    GM_SOURCE_SYSTEM      = 4,    /* System-generated (e.g., AUGMENTS) */
    GM_SOURCE_IMPORT      = 5,    /* Imported from external source */
    GM_SOURCE_UNKNOWN     = 255   /* Unknown source */
} gm_source_type_t;
```

### Attribution Metadata

Each edge carries attribution information:
- `source_type`: Who created it
- `author`: Email or identifier
- `session_id`: For grouping related edges
- `flags`: Review status, consensus, etc.

### Lanes

Edges can be organized into lanes for different purposes:
- `GM_LANE_ARCHITECTURE`: Architecture documentation
- `GM_LANE_TESTING`: Test coverage analysis
- `GM_LANE_ANALYSIS`: AI analysis results

## Usage

### Setting Attribution

```c
gm_attribution_t attr;
gm_attribution_from_env(&attr);  // Get from environment

// Or set explicitly
gm_attribution_set_default(&attr, GM_SOURCE_AI_CLAUDE);
```

### Filtering Edges

```c
gm_filter_t filter;

// Show only human edges
gm_filter_init_human_only(&filter);

// Show AI insights with high confidence
gm_filter_init_ai_insights(&filter, 0.8);

// Check if edge matches
if (gm_filter_match(&filter, edge)) {
    // Process edge
}
```

## Storage Format

Attribution data is encoded in CBOR along with edge data:
```
[
  source_sha, tgt_sha, rel_type, confidence, timestamp,
  src_path, tgt_path, ulid,
  source_type, author, session_id, flags, lane  // New fields
]
```

## Environment Variables

- `GIT_MIND_SOURCE`: Set source type (human, claude, gpt, system)
- `GIT_MIND_AUTHOR`: Override author identification
- `GIT_MIND_SESSION`: Group edges in same session

## Integration with MCP

When Claude creates edges via MCP:
```bash
export GIT_MIND_SOURCE=claude
export GIT_MIND_AUTHOR=claude@anthropic
export GIT_MIND_SESSION=conv_abc123
```

This ensures all edges created in that session are properly attributed.