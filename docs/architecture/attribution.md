---
title: Attribution System
description: Overview of the attribution domain and data model.
audience: [developers]
domain: [architecture]
tags: [attribution]
status: draft
---

<!-- SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 -->
<!-- © 2025 J. Kirby Ross / Neuroglyph Collective -->

# Attribution System

## Table of Contents

- [Core Concepts](#core-concepts)
- [Integration Guide](#integration-guide)

This document provides a comprehensive overview of the `git-mind` attribution system, which is the foundation for human-AI collaboration. It tracks who created each semantic edge, enabling filtered views, collaborative workflows, and consensus building.

## Core Concepts

### Source Types

Every edge must have a source indicating its creator:

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

```c
typedef struct {
    gm_source_type_t source_type;       /* Who created this edge */
    char author[64];                    /* Email or identifier */
    char session_id[32];                /* Session/conversation ID */
    uint32_t flags;                     /* Review status and metadata */
} gm_attribution_t;
```

### Attribution Flags

Flags track the review and consensus status:

```c
#define GM_ATTR_REVIEWED    0x0001      /* Human reviewed AI suggestion */
#define GM_ATTR_ACCEPTED    0x0002      /* AI suggestion accepted */
#define GM_ATTR_REJECTED    0x0004      /* AI suggestion rejected */
#define GM_ATTR_MODIFIED    0x0008      /* AI suggestion modified */
#define GM_ATTR_CONSENSUS   0x0010      /* Human and AI agree */
#define GM_ATTR_CONFLICT    0x0020      /* Human and AI disagree */
#define GM_ATTR_PENDING     0x0040      /* Awaiting review */
```

## Integration Guide

### For CLI Commands

When creating an edge in a CLI command, you should get the attribution information from the environment:

```c
#include "gitmind/attribution.h"

// In your command handler:
gm_edge_attributed_t edge = {0};
gm_attribution_t attr;

// Get attribution from environment
gm_attribution_from_env(&attr);
edge.attribution = attr;

// If AI is creating edge, get confidence from args
if (edge.attribution.source_type != GM_SOURCE_HUMAN) {
    edge.confidence = parse_confidence(args);
} else {
    edge.confidence = 0x3C00;  // 1.0 for humans
}

// Encode with attribution
gm_edge_attributed_encode_cbor(&edge, buffer, &len);
```

### Environment Variables

*   **`GIT_MIND_SOURCE`**: `human`, `claude`, `gpt`, `system`, `import`, `other`
*   **`GIT_MIND_AUTHOR`**: An identifier for the author (e.g., `user@example.com`)
*   **`GIT_MIND_SESSION`**: An identifier for the current session or conversation.

## Use Cases

### AI-Assisted Code Review

A developer can ask an AI to analyze the codebase for hidden dependencies. The AI can then create `suggested` edges with a `pending` flag. The developer can then review these suggestions and either accept, reject, or modify them.

### Onboarding

A new developer can start by exploring the human-curated edges to understand the core architecture. They can then explore the AI-generated edges to discover more subtle relationships and patterns in the code.

### Collaborative Architecture Documentation

A team lead can document the high-level architecture by creating edges. An AI can then be used to fill in the details by analyzing the codebase and suggesting more granular relationships. The team can then review and approve the AI's suggestions.
