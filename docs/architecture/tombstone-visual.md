# Visual Guide: How Tombstones Work

Table of Contents
- [The Problem: Traditional Deletion](#the-problem-traditional-deletion)
- [The Solution: Tombstones](#the-solution-tombstones)

## The Problem: Traditional Deletion

```mermaid
graph LR
    subgraph "Before Delete"
        A1[File A] -->|link| B1[File B]
    end
    
    subgraph "After Delete"
        A2[File A] -.->|???| B2[File B]
        X[Link Gone!<br/>No History!]
    end
    
    style X fill:#ff6b6b
```

## The Solution: Tombstones

```mermaid
graph TD
    subgraph "Time 1: Create Link"
        T1[10:00 AM]
        E1[Edge 1<br/>confidence: +1.0<br/>✅ Shows in list]
    end
    
    subgraph "Time 2: Unlink (Tombstone)"
        T2[11:00 AM]
        E1_2[Edge 1<br/>confidence: +1.0]
        E2[Edge 2<br/>confidence: -1.0<br/>❌ Hidden from list]
    end
    
    subgraph "Time 3: Re-link"
        T3[12:00 PM]
        E1_3[Edge 1<br/>confidence: +1.0]
        E2_3[Edge 2<br/>confidence: -1.0]
        E3[Edge 3<br/>confidence: +1.0<br/>✅ Shows in list]
    end
    
    T1 --> T2
    T2 --> T3
    
    style E1 fill:#90EE90
    style E2 fill:#FFB6C1
    style E3 fill:#90EE90
```

## How List Filtering Works

```mermaid
flowchart LR
    subgraph "Git Tree"
        E1[Edge: A→B<br/>conf: 1.0<br/>time: 10:00]
        E2[Edge: A→B<br/>conf: -1.0<br/>time: 11:00]
        E3[Edge: C→D<br/>conf: 1.0<br/>time: 10:30]
        E4[Edge: E→F<br/>conf: -1.0<br/>time: 11:30]
        E5[Edge: E→F<br/>conf: 1.0<br/>time: 12:00]
    end
    
    subgraph "Filter Logic"
        F{confidence > 0?}
    end
    
    subgraph "User Sees"
        L1[A → B ❌ Hidden]
        L2[C → D ✅ Visible]
        L3[E → F ✅ Visible]
    end
    
    E1 --> F
    E2 --> F
    E3 --> F
    E4 --> F
    E5 --> F
    
    F -->|Yes| L2
    F -->|Yes| L3
    F -->|No| L1
    
    style E2 fill:#FFB6C1
    style E4 fill:#FFB6C1
    style L1 fill:#f8d7da
    style L2 fill:#d4edda
    style L3 fill:#d4edda
```

## Real Example in Git

```mermaid
gitGraph
    commit id: "init"
    
    branch refs/gitmind/graph
    commit id: "link A→B created" tag: "confidence: 1.0"
    commit id: "link C→D created" tag: "confidence: 1.0"
    commit id: "unlink A→B" tag: "confidence: -1.0" type: REVERSE
    commit id: "link A→B recreated" tag: "confidence: 1.0"
```

## Tree Structure with Tombstones

```mermaid
graph TD
    Root[refs/gitmind/graph]
    Root --> SrcFan1[ab/cd/]
    SrcFan1 --> Src1[abcdef123.../]
    Src1 --> Type1[implements/]
    Type1 --> EdgeFan1[01/23/]
    EdgeFan1 --> Edge1[ULID_1<br/>✅ conf: 1.0<br/>10:00 AM]
    EdgeFan1 --> Edge2[ULID_2<br/>❌ conf: -1.0<br/>11:00 AM<br/>TOMBSTONE]
    EdgeFan1 --> Edge3[ULID_3<br/>✅ conf: 1.0<br/>12:00 PM]
    
    style Root fill:#4CAF50
    style Edge1 fill:#90EE90
    style Edge2 fill:#FFB6C1
    style Edge3 fill:#90EE90
```

## Benefits Visualized

```mermaid
graph TD
    subgraph "Benefits"
        H[📜 Complete History]
        R[♻️ Resurrection Possible]
        M[🤝 Merges Cleanly]
        A[🔍 Audit Trail]
    end
    
    subgraph "Implementation"
        T[Tombstone<br/>confidence: -1.0]
    end
    
    T --> H
    T --> R
    T --> M
    T --> A
    
    style T fill:#FFB6C1
    style H fill:#E3F2FD
    style R fill:#F3E5F5
    style M fill:#FFF9C4
    style A fill:#E8F5E9
```

## Code Behavior

```mermaid
sequenceDiagram
    participant User
    participant API
    participant Storage
    
    Note over User,Storage: Creating a link
    User->>API: gm_link_create("A", "B", "tests")
    API->>Storage: Store edge (confidence: 1.0)
    Storage-->>API: Success
    API-->>User: Link created
    
    Note over User,Storage: Listing shows the link
    User->>API: gm_link_list()
    API->>Storage: Get all edges
    Storage-->>API: Returns edge (conf: 1.0)
    API->>API: Filter: conf > 0 ✓
    API-->>User: Shows: A → B
    
    Note over User,Storage: Unlinking creates tombstone
    User->>API: gm_link_unlink("A", "B")
    API->>Storage: Store tombstone (confidence: -1.0)
    Storage-->>API: Success
    API-->>User: Link removed
    
    Note over User,Storage: Listing hides tombstoned link
    User->>API: gm_link_list()
    API->>Storage: Get all edges
    Storage-->>API: Returns both edges
    API->>API: Filter: conf > 0 ✗ for tombstone
    API-->>User: Shows: (empty)
```

## Summary

Tombstones are edges with **negative confidence** that:
- 📜 Preserve complete history
- 👻 Make links "disappear" from user view
- 🔄 Allow clean distributed merges
- ♻️ Enable future resurrection
- 🔍 Provide audit trails

The user sees the **behavior** (link gone) without knowing the **implementation** (tombstone edge)!
