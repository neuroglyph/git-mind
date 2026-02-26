# Contributing to git-mind

Thanks for your interest in contributing. This document covers the essentials.

## Prerequisites

- Node.js >= 20.0.0
- Git
- A local clone of [`@git-stunts/git-warp`](https://github.com/nicktomlin/git-warp) (git-mind depends on it via local path)

## Setup

```bash
git clone https://github.com/neuroglyph/git-mind.git
cd git-mind
npm install
npm test
```

## Making changes

1. Create a branch from `main` (or the current development branch)
2. Make your changes
3. Run the tests: `npm test`
4. Commit using [Conventional Commits](https://www.conventionalcommits.org/):
   - `feat:` — new feature
   - `fix:` — bug fix
   - `docs:` — documentation only
   - `chore:` — maintenance, tooling
   - `refactor:` — code change that neither fixes a bug nor adds a feature
   - `test:` — adding or updating tests
5. Open a pull request

## Commit messages

Follow the conventional commit format:

```
type(scope): short description

Longer explanation if needed.

Refs #issue-number
```

Every commit should reference a GitHub issue. If one doesn't exist, create it.

## Code style

- Plain JavaScript with JSDoc type annotations (no TypeScript)
- ES modules (`import`/`export`)
- Format with Prettier: `npm run format`
- Lint with ESLint: `npm run lint`

## Tests

Tests use [vitest](https://vitest.dev/). Run them with:

```bash
npm test          # single run
npm run test:watch  # watch mode
```

Each test creates a temporary Git repo in the OS temp directory and cleans up after itself.

## Project structure

```
bin/git-mind.js      — CLI entry point
src/
  graph.js           — WarpGraph wrapper (init, load, save)
  edges.js           — Edge CRUD with types + confidence
  views.js           — Observer view definitions and rendering
  hooks.js           — Post-commit directive parser
  cli/
    commands.js      — CLI command implementations
    format.js        — Terminal output formatting
  index.js           — Public API exports
test/
  graph.test.js      — Graph lifecycle tests
  edges.test.js      — Edge CRUD tests
  views.test.js      — View filtering tests
  hooks.test.js      — Directive parsing tests
```

## Public API & Deprecation Protocol

git-mind's public API is everything exported from `src/index.js`. A stability
test (`test/api-surface.test.js`) snapshots every export name and type — CI
will fail if the surface changes without an intentional update.

### Making changes to the public API

| Change | Semver | Process |
|--------|--------|---------|
| **Add** an export | minor | Add to `API_SNAPSHOT` in `test/api-surface.test.js` |
| **Remove** an export | **major** | Follow the deprecation protocol below |
| **Change** an export's type/signature | **major** | Follow the deprecation protocol below |

### Deprecation protocol

1. **Mark deprecated** — Add `@deprecated` JSDoc tag with a migration note and
   the target removal version (at least one minor release away):
   ```js
   /** @deprecated Use newFunction() instead. Removal: v6.0.0 */
   export function oldFunction() { ... }
   ```
2. **Runtime warning** — Emit a one-time `console.warn` on first call:

   ```text
   [git-mind] oldFunction() is deprecated — use newFunction(). Removal: v6.0.0
   ```

3. **Keep in snapshot** — The export stays in `test/api-surface.test.js` until
   the major version that removes it.
4. **Remove** — In the next major version, delete the export, remove it from
   the snapshot, and document the removal in CHANGELOG.md.

## License

By contributing, you agree that your contributions will be licensed under [Apache-2.0](LICENSE).
