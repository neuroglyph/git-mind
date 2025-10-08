# .trash (Quarantined Docs)

This folder holds files moved out of the primary documentation tree. Each entry includes a rationale. Items can be restored or deleted after review.

Current status: The following files are quarantined.

- scripts/neo4j-curl.sh — Local Neo4j HTTP helper; Neo4j not part of git-mind
- scripts/gm-neo4j-upsert-edge.sh — Local Neo4j upsert script; out of scope
- scripts/neo4j-show-task.sh — Local Neo4j task query helper; out of scope
- scripts/neo4j-export-edges.sh — Local Neo4j export helper; out of scope
- scripts/neo4j-constraints.json — Neo4j constraints payload; out of scope
- scripts/neo4j-task-gm-docker-neo4j-2025-09-12.json — Example payload; out of scope
- 0_build-test.txt — Build output scratch file; not part of sources
- ci_logs.zip — Compressed CI logs; keep local only
- Untitled.md — Empty placeholder; safe to remove
- test_edge_compile_check — Local test binary/probe; belongs in ignored build outputs
- SITREP_$(date +%s)_journal_attribution_migration_complete.md — Misnamed template SITREP (literal shell in filename); archived to avoid confusion

Format for entries:

- <path> — <reason>

Example:

- docs/old-guide.md — superseded by docs/architecture/System_Architecture.md
