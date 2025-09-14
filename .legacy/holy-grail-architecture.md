# Git-Mind: Holy Grail Architecture

## Overview

Git-mind stores semantic relationships using Git's native object model. No external database. No custom formats. Just trees and blobs.

```mermaid
graph TD
    A[Working Directory] -->|never touches| X[❌]
    B[refs/gitmind/graph] -->|orphan ref| C[Root Tree]
    C --> D[Source Trees]
    D --> E[Relationship Trees]
    E --> F[Edge Blobs]
    
    style B fill:#9f9,stroke:#393,stroke-width:4px
    style F fill:#f9f,stroke:#939,stroke-width:2px
```

## Core Concept: Everything is Content-Addressable

```mermaid
graph LR
    F1[parser.c] -->|SHA: abc123| S1[Source Tree]
    F2[grammar.md] -->|SHA: def456| S2[Target SHA in Edge]
    
    S1 -->|contains| RT[Relationship Tree]
    RT -->|contains| E[Edge Blob]
    E -->|points to| S2
    
    style F1 fill:#e1f5fe
    style F2 fill:#fff3e0
    style E fill:#f3e5f5
```

## The Double Fan-out Structure

```mermaid
graph TD
    subgraph "refs/gitmind/graph"
        R[Root Tree]
    end
    
    subgraph "Source Fan-out"
        R --> SF1[ab/]
        SF1 --> SF2[cd/]
        SF2 --> S[abc123.../]
    end
    
    subgraph "Relationship Trees"
        S --> RT1[implements/]
        S --> RT2[references/]
        S --> RT3[tests/]
    end
    
    subgraph "Edge Fan-out"
        RT1 --> EF1[01/]
        EF1 --> EF2[23/]
        EF2 --> E1[0123ABCD...ULID.cbor]
        EF2 --> E2[0123EFGH...ULID.cbor]
    end
    
    style R fill:#9f9
    style E1 fill:#f9f
    style E2 fill:#f9f
```

## Why Double Fan-out

```mermaid
graph LR
    subgraph "Without Fan-out"
        T1[Tree with 1M entries] -->|O(n) lookup| SLOW[❌ Slow]
    end
    
    subgraph "With Fan-out"
        T2[Tree ≤ 256 entries] -->|O(1) lookup| FAST[✅ Fast]
    end
    
    style SLOW fill:#fcc,stroke:#f00
    style FAST fill:#cfc,stroke:#0f0
```

## Edge Blob Format (CBOR)

```mermaid
graph TD
    subgraph "CBOR Edge Blob"
        M[Map Header]
        M --> T[0x00: target_sha<br/>20 bytes]
        M --> C[0x01: confidence<br/>half-float]
        M --> TS[0x02: timestamp<br/>uint64]
        M --> E[0x03: extra<br/>optional map]
    end
    
    subgraph "Size: 16-40 bytes"
        COMP[Compressed by Git]
    end
    
    style M fill:#ffd
    style T fill:#ddf
    style C fill:#dfd
```

## Creating a Link

```mermaid
sequenceDiagram
    participant CLI as git mind link
    participant Core as git-mind
    participant Git as Git
    
    CLI->>Core: link(source, target, type)
    Core->>Git: hash-object source
    Git-->>Core: source SHA
    Core->>Git: hash-object target
    Git-->>Core: target SHA
    Core->>Core: Generate ULID
    Core->>Core: Encode CBOR edge
    Core->>Git: hash-object -w (CBOR blob)
    Git-->>Core: edge SHA
    Core->>Core: Compute fan-out path
    Core->>Git: mktree (build tree structure)
    Git-->>Core: new tree SHA
    Core->>Git: update-ref refs/gitmind/graph
    Git-->>Core: success
    Core-->>CLI: link created
```

## Querying Outgoing Links

```mermaid
sequenceDiagram
    participant CLI as git mind list
    participant Core as git-mind
    participant Git as Git
    
    CLI->>Core: list(source)
    Core->>Git: hash-object source
    Git-->>Core: source SHA
    Core->>Core: Compute fan-out path
    Core->>Git: ls-tree refs/gitmind/graph:path
    Git-->>Core: relationship trees
    
    loop For each relationship
        Core->>Git: ls-tree -r (relationship)
        Git-->>Core: edge blobs
        
        loop For each edge
            Core->>Git: cat-file blob (edge)
            Git-->>Core: CBOR data
            Core->>Core: Decode CBOR
        end
    end
    
    Core-->>CLI: list of edges
```

## Why This Architecture Wins

```mermaid
graph TD
    subgraph "Traditional Approach"
        F[Files in .gitmind/] -->|Problem 1| P1[Working tree pollution]
        F -->|Problem 2| P2[Merge conflicts]
        F -->|Problem 3| P3[Poor performance]
    end
    
    subgraph "Holy Grail Approach"
        O[Orphan ref] -->|Solution 1| S1[No working tree files]
        T[Trees] -->|Solution 2| S2[Unique paths = no conflicts]
        B[Blobs] -->|Solution 3| S3[Git compression + O(1) lookups]
    end
    
    style P1 fill:#fcc
    style P2 fill:#fcc
    style P3 fill:#fcc
    style S1 fill:#cfc
    style S2 fill:#cfc
    style S3 fill:#cfc
```

## The Complete Architecture

```mermaid
graph TB
    subgraph "Git Repository"
        WD[Working Directory]
        ODB["(Object Database)"]
        REFS[References]
    end
    
    subgraph "Git-Mind Structure"
        REFS --> GMR[refs/gitmind/graph]
        GMR --> RT[Root Tree]
        
        RT --> FO1[Source Fan-out Trees]
        FO1 --> ST[Source Trees]
        ST --> REL[Relationship Trees]
        REL --> FO2[Edge Fan-out Trees]
        FO2 --> EDGES[Edge Blobs]
        
        EDGES -.->|points to| ODB
    end
    
    subgraph "Key Properties"
        P1[No working tree pollution]
        P2[Content-addressable]
        P3[Merge-conflict free]
        P4["O(1) performance"]
        P5[Git-native compression]
    end
    
    style GMR fill:#9f9,stroke:#393,stroke-width:4px
    style EDGES fill:#f9f,stroke:#939
    style P1 fill:#cfc
    style P2 fill:#cfc
    style P3 fill:#cfc
    style P4 fill:#cfc
    style P5 fill:#cfc
```

## Evolution: File-based to Tree-based

```mermaid
graph LR
    subgraph "Old World"
        OF[.gitmind/links/*.link files]
        OF -->|Problems| OP[Pollution, Conflicts, Scale]
    end
    
    subgraph "Transition"
        Q["Why not use Git's trees?"]
    end
    
    subgraph "New World"
        NT[refs/gitmind/graph trees]
        NT -->|Benefits| NB[Clean, Conflict-free, Fast]
    end
    
    OF -.->|Revelation| Q
    Q -.->|Implementation| NT
    
    style Q fill:#ffd,stroke:#000,stroke-width:3px
    style NB fill:#9f9
```

## The Beauty

No external dependencies. No custom object types. No daemons. Just Git.

Every operation uses Git plumbing. Every structure is a Git object. Every guarantee comes from Git itself.

This isn't a graph database built ON Git. This IS Git, seen correctly.

---

_"Perfection is achieved not when there is nothing more to add, but when there is nothing left to take away."_

The graph breathes. The edges live. Understanding accumulates. Welcome to Tuxville. 🐧
