# Repository Guidelines

## Project Structure & Module Organization
- `core/` — C23 library: `include/` (public headers), `src/` (impl), `tests/` (unit). Add new code here.
- `include/` — Umbrella API (`gitmind.h`) and namespaced headers under `include/gitmind/`.
- `src/` — Legacy code under migration. Avoid adding new modules here.
- `apps/` — Future CLI/hooks/apps. Until announced, prefer `core/` additions.
- `tests/` — E2E/integration/legacy tests and fixtures.
- `tools/`, `quality/`, `.githooks/` — Dev tooling (clang-tidy, formatting, gauntlet, secret scanning).
- `docs/` — Architecture, dev setup, and operational notes.

## Build, Test, and Development Commands
- Configure + build: `meson setup build && ninja -C build`
- Run unit tests: `ninja -C build test` (or `meson test -C build`)
- Make shims: `make`, `make test`, `make clean`
- Lint (CI parity): `./tools/docker-clang-tidy.sh` → produces `clang-tidy-report.txt`
- Strict multi-compiler build: `./tools/gauntlet/run-gauntlet.sh` (recommended before PRs)
- Enable hooks: `pre-commit install` (clang-format + detect-secrets)

## Coding Style & Naming Conventions
- Language: C23 with warnings-as-errors; no VLAs or shadowing; explicit prototypes.
- Formatting: `.clang-format` (LLVM-based, 4 spaces, 80 cols, pointer alignment right). Pre-commit runs `clang-format`.
- Naming: functions/vars `lower_snake_case` (prefix `gm_`), macros `UPPER_SNAKE`, types end in `_t`, header guards `GITMIND_*`.
- Includes: prefer specific headers; order/regroup per `.clang-format`.

## Testing Guidelines
- Unit tests live in `core/tests/unit/` as `test_<module>.c`. Keep deterministic and isolated.
- Meson wires test backends as needed—just build and run tests via Meson/Ninja.
- E2E/integration under `tests/` for CLI/flow checks. Keep fixtures minimal.

## Commit & Pull Request Guidelines
- Conventional commits: `type(scope): description` (e.g., `fix(core/cbor): handle null keys`). Reference issues (`Fixes #123`).
- PRs must describe changes, link issues, include a short test plan, pass CI, and introduce no new clang-tidy warnings (`./tools/docker-clang-tidy.sh`). Update docs when applicable.

## Security & Configuration
- Do not commit secrets. Pre-commit runs `detect-secrets` with `.secrets.baseline`.
- Use `.env.example` as a template; never check in real credentials.

## Agent-Specific Notes
- Make minimal, focused diffs; avoid drive-by refactors. Match existing patterns in `core/` and headers under `include/`.
- Run build, tests, and lint locally before proposing changes. Keep changes zero-warnings and formatted.

## Neo4j Codex Log (Memory Graph)
- Purpose: record tasks, touched files, and relationships to enable impact/risk queries for this repo.
- Project tagging: all nodes carry label `:GM` and `repo:'git-mind'`.
- Setup env:
  - `export NEO4J_HTTP_URL=http://localhost:7474`
  - `export NEO4J_USER=neo4j`
  - `export NEO4J_PASSWORD=password123` (use real local creds; don’t commit)
- One‑time constraints (HTTP):
  - Use `scripts/neo4j-constraints.json` which creates GM-scoped constraints (Task:GM.id unique, File:GM.path unique, repo existence).
- Helper script: `scripts/neo4j-curl.sh`
  - Pipe JSON or use `-f payload.json`. Posts to `NEO4J_HTTP_URL/db/neo4j/tx/commit` with basic auth.
- Quick start examples:
  - Start task: `echo '{"statements":[{"statement":"MERGE (t:Task:GM {id:$id}) ON CREATE SET t.title=$title, t.status=\"in_progress\", t.started_at:datetime(), t.repo=\"git-mind\" ON MATCH SET t.status=\"in_progress\", t.repo=\"git-mind\"","parameters":{"id":"gm-docker-namespacing","title":"Docker image names + cleanup"}}]}' | bash scripts/neo4j-curl.sh`
  - Link touched file: `echo '{"statements":[{"statement":"MERGE (t:Task:GM {id:$id}) MERGE (f:File:GM {path:$p}) ON CREATE SET f.repo=\"git-mind\" ON MATCH SET f.repo=\"git-mind\" MERGE (t)-[:TOUCHES]->(f)","parameters":{"id":"gm-docker-namespacing","p":"tools/docker-clean.sh"}}]}' | bash scripts/neo4j-curl.sh`
  - Record dependency: `echo '{"statements":[{"statement":"MERGE (a:File:GM {path:$a}) ON CREATE SET a.repo=\"git-mind\" ON MATCH SET a.repo=\"git-mind\" MERGE (b:File:GM {path:$b}) ON CREATE SET b.repo=\"git-mind\" ON MATCH SET b.repo=\"git-mind\" MERGE (a)-[:DEPENDS_ON]->(b)","parameters":{"a":"tools/docker-clang-tidy.sh","b":".ci/Dockerfile"}}]}' | bash scripts/neo4j-curl.sh`

### Ready-to-run payloads in this repo
- Initialize constraints (once): `bash scripts/neo4j-curl.sh -f scripts/neo4j-constraints.json`
- Log this task (files touched): `bash scripts/neo4j-curl.sh -f scripts/neo4j-task-gm-docker-neo4j-2025-09-12.json`
- Verify touched files for a task: `scripts/neo4j-show-task.sh gm-docker-namespacing-neo4j-2025-09-12`

## Neo4j Proto Edge Schema (git-mind semantics)
- Relationship: `(a:File:GM {repo:'git-mind'})-[:EDGE {ulid, type, lane, confidence, ts, commit, src_path, tgt_path, src_sha, tgt_sha, author, source_type, session_id, repo:'git-mind'}]->(b:File:GM {repo:'git-mind'})`
- Properties map 1:1 to libgitmind edge fields (ulid, attribution, lane, SHAs, etc.).
- Upsert example (script):
  - `scripts/gm-neo4j-upsert-edge.sh --commit HEAD --src core/src/io/io.c --tgt core/include/gitmind/io/io.h --type IMPLEMENTS --lane verified --author user@local --source human --confidence 90`
- Export edges to JSON/NDJSON:
  - `scripts/neo4j-export-edges.sh` (uses jq if available)

### Importer plan (proto → journal)
- Extract edges: `scripts/neo4j-export-edges.sh > edges.json`
- For each edge row:
  - Resolve paths to blob OIDs at `edge.commit` (already present as `src_sha`/`tgt_sha`)
  - Write journal entry under `refs/gitmind/edges/<lane>` with full properties (ulid, attribution, type)
  - Rebuild cache: `gm_cache_rebuild()` (Meson test harness covers core APIs)
- Validate parity: compare fanout/fanin against Neo4j MATCH results on the same commit.
