# git-mind v2 — Bootstrap Plan

This file is the instruction set for the new Claude Code instance working in this repo.

**Read this entire file before doing anything.**

---

## Context

git-mind is being repurposed. The C23 codebase (hexagonal architecture, Meson build,
libgit2, libsodium, CBOR) served its purpose but is being replaced by a thin Node.js
project built on top of `@git-stunts/git-warp` — a multi-writer CRDT graph database
on Git that already provides everything git-mind was building from scratch.

The canonical roadmap for why this is happening lives in `~/git/echo/docs/ROADMAP.md`
(Phase 0: git-mind v2). The session that led to this decision is documented in
`~/git/echo/docs/EPIC_SESSION.md`.

---

## Step 1: Preserve the C23 codebase

1. You are currently on branch `chore/temp-port-ci-copy` with uncommitted changes.
2. Stage ALL changes (modified + untracked files). Use `git add -A`.
3. Commit with message: `chore: final C23 snapshot before v2 rewrite`
4. Push this branch: `git push origin chore/temp-port-ci-copy`
5. Switch to `main`.
6. Create and push an archive branch: `git branch archive/c23-final && git push origin archive/c23-final`
   This preserves the C23 codebase permanently on a named branch.

## Step 2: Create git-mind-v2 branch and wipe

1. From `main`, create and checkout: `git checkout -b git-mind-v2`
2. Remove ALL tracked files EXCEPT these (preserve them exactly):
   - `.gitignore`
   - `.gitattributes`
   - `LICENSE`
   - `NOTICE`
   - `.reuse/` (directory, if it exists)
3. Remove this plan file too (`V2_PLAN.md`) — it will have served its purpose.
4. Commit: `chore: clean slate for git-mind v2`

## Step 3: Initialize the new project

### package.json

Create `package.json`:

```json
{
  "name": "@neuroglyph/git-mind",
  "version": "2.0.0-alpha.0",
  "description": "A project knowledge graph tool built on git-warp",
  "type": "module",
  "license": "SEE LICENSE IN LICENSE",
  "author": "James Ross <james@flyingrobots.dev>",
  "repository": {
    "type": "git",
    "url": "https://github.com/neuroglyph/git-mind.git"
  },
  "bin": {
    "git-mind": "bin/git-mind.js"
  },
  "exports": {
    ".": {
      "types": "./src/index.d.ts",
      "default": "./src/index.js"
    }
  },
  "scripts": {
    "test": "vitest run",
    "test:watch": "vitest",
    "lint": "eslint src/ bin/",
    "format": "prettier --write 'src/**/*.js' 'bin/**/*.js'"
  },
  "dependencies": {
    "@git-stunts/git-warp": "^8.0.0",
    "chalk": "^5.3.0",
    "figures": "^6.0.1"
  },
  "devDependencies": {
    "vitest": "^3.0.0",
    "prettier": "^3.0.0",
    "eslint": "^9.0.0"
  },
  "engines": {
    "node": ">=20.0.0"
  }
}
```

**IMPORTANT**: `@git-stunts/git-warp` is a local package at `~/git/git-stunts/git-warp`.
After creating package.json, install it with:
```bash
npm install ~/git/git-stunts/git-warp
```
This creates a symlink. Don't `npm install` from the registry — it's not published.

### .gitignore

The existing `.gitignore` is already Node.js-ready. Trim out the C23-specific sections
(`.o`, `.a`, `.so`, `.dylib`, `c/build/`, Meson/CMake stuff, etc.) but keep the
Node.js, TypeScript, VS Code, and general sections. Add:
```
# git-mind v2 specific
.gitmind/
```

### Project structure

```
bin/
  git-mind.js          # CLI entry point (#!/usr/bin/env node)
src/
  index.js             # Public API
  graph.js             # WarpGraph wrapper — init, load, save
  edges.js             # Edge creation with types + confidence
  views.js             # Observer view definitions and rendering
  hooks.js             # Post-commit hook: parse directives from commit messages
  cli/
    commands.js         # Command registry (init, link, view, suggest, review)
    format.js           # Terminal output formatting
test/
  graph.test.js
  edges.test.js
  views.test.js
  hooks.test.js
```

## Step 4: Implement core modules

### bin/git-mind.js

Minimal CLI entry point. Parses `process.argv` for subcommands:
- `git mind init` → `commands.init()`
- `git mind link <source> <target> --type <type>` → `commands.link()`
- `git mind view <name>` → `commands.view()`
- `git mind suggest --ai` → (stub for now, prints "AI suggestions not yet implemented")
- `git mind review` → (stub for now)

Use `process.argv` directly — no commander/yargs dependency. Keep it minimal.

### src/graph.js

Wraps `@git-stunts/git-warp` graph operations:
- `initGraph(repoPath)` — initialize a WARP graph in `.gitmind/`
- `loadGraph(repoPath)` — load existing graph
- `saveGraph(graph)` — persist to Git

The WARP graph stores:
- **Nodes**: project artifacts (files, crates, specs, phases, tasks, concepts)
- **Edges**: semantic relationships with metadata (type, confidence, source, timestamp)

### src/edges.js

Edge creation and querying:
- `createEdge(graph, { source, target, type, confidence, rationale })`
- `queryEdges(graph, { source?, target?, type? })`
- `removeEdge(graph, edgeId)`

Edge types: `implements`, `augments`, `relates-to`, `blocks`, `belongs-to`,
`consumed-by`, `depends-on`, `documents`.

Confidence: `0.0` to `1.0`. AI-created edges start at low confidence (e.g. 0.3).
Human-created edges default to `1.0`. Human review can adjust.

### src/views.js

Observer views — filtered projections of the graph:
- `defineView(name, filterFn)` — register a named view
- `renderView(graph, viewName)` — apply filter, format output
- Built-in views: `roadmap`, `backlog`, `architecture`, `suggestions`

A view is just a filter function: `(nodes, edges) => filteredSubgraph`.
The `roadmap` view filters for nodes of type `phase` or `task` with status != `done`.
The `architecture` view filters for `crate` and `depends-on` edges.

### src/hooks.js

Post-commit hook parser:
- Scans commit messages for directives: `IMPLEMENTS: <target>`, `AUGMENTS: <target>`,
  `RELATES-TO: <target>`
- Auto-creates edges with confidence `0.8` (high but not human-reviewed)
- Records the commit SHA as provenance

### src/cli/commands.js

Wire commands to the core modules. Each command:
1. Loads the graph
2. Performs the operation
3. Saves if modified
4. Prints formatted output

### src/cli/format.js

Terminal formatting with chalk. Keep it clean and minimal.

## Step 5: Write tests

Write tests for each module using vitest. Focus on:
- Graph init/load/save round-trip
- Edge CRUD operations
- View filtering
- Hook directive parsing (unit test with mock commit messages)

Use a temporary directory for test graphs — don't pollute the repo.

## Step 6: Commit and push

After everything is working:
1. `git add -A`
2. Commit: `feat: git-mind v2 — project knowledge graph on git-warp`
3. Push: `git push -u origin git-mind-v2`

---

## Design principles

1. **git-warp does the heavy lifting.** Don't reimplement CRDT merge, observer views,
   temporal queries, or provenance. Use git-warp's APIs.
2. **Minimal dependencies.** chalk for colors, figures for symbols, git-warp for
   everything else. No frameworks.
3. **Plain JavaScript with JSDoc.** No TypeScript build step. Use `.js` files with
   JSDoc type annotations. git-warp itself does this successfully.
4. **Git-native.** The graph lives in Git. No external databases. `git mind init`
   creates a WARP graph in `.gitmind/`. Everything is committed to Git history.
5. **Progressive enhancement.** AI suggestions are a stub now. The core (edges, views,
   hooks) works without AI. AI features are added later via the `suggest` and `review`
   commands.

---

## What NOT to do

- Don't install commander, yargs, or any CLI framework. Parse argv manually.
- Don't add TypeScript. Use plain JS with JSDoc.
- Don't create a monorepo or workspace structure. Single package.
- Don't implement AI suggestions yet. Stub the commands.
- Don't over-engineer. This is a ~1-2 day project. Keep it tight.
