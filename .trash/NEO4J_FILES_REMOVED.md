# Neo4j-related files removed from scope

These files were quarantined because Neo4j is not part of git-mind. They were only used for local experiments and should live outside this repository.

- scripts/neo4j-curl.sh — Local HTTP helper for Neo4j transactional API
- scripts/gm-neo4j-upsert-edge.sh — Upsert edges into a Neo4j instance
- scripts/neo4j-show-task.sh — Query helper for task/file relationships
- scripts/neo4j-export-edges.sh — Export edges from Neo4j to JSON
- scripts/neo4j-constraints.json — Local constraint payload for experiments
- scripts/neo4j-task-gm-docker-neo4j-2025-09-12.json — Local task payload example

Fate: Keep quarantined; delete once confirmed no longer needed. If you want to use them locally, maintain them outside this repo.
