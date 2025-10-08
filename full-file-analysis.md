# Full File Hierarchy Analysis (Detailed)

This document provides a comprehensive, file-by-file analysis of the entire file hierarchy of the `git-mind` repository. The goal is to document the purpose of each file, its relevance to the project's hexagonal architecture, and to recommend actions for cleanup and refactoring.

## Root Directory Files

| Filepath | What? | Why? | Critical? | Hexagonal Architecture OK? | Recommended Action | Remarks | Notes |
|---|---|---|---|---|---|---|---|
| `.clang-format` | Configuration for `clang-format`. | Enforces a consistent C code style. | ✅ Yes | N/A | Keep | Essential for code quality. | |
| `.coderabbit.yml` | Configuration for CodeRabbit. | Automates parts of the code review process. | ✅ Yes | N/A | Keep | Important for development workflow. | |
| `.dockerignore` | Specifies files to ignore in Docker builds. | Reduces Docker image size and build time. | ✅ Yes | N/A | Keep | Essential for efficient Docker builds. | |
| `.env.example` | Example environment file. | Provides a template for environment variables. | ✅ Yes | N/A | Keep | Important for new contributors. | |
| `.gitattributes` | Defines attributes for paths. | Configures Git behavior for specific files. | ✅ Yes | N/A | Keep | Important for repository correctness. | |
| `.gitignore` | Specifies files to be ignored by Git. | Prevents committing generated files and secrets. | ✅ Yes | N/A | Keep | Essential for repository hygiene. | |
| `.markdownlint.jsonc` | Configuration for `markdownlint`. | Enforces a consistent style for Markdown files. | ✅ Yes | N/A | Keep | Important for documentation quality. | |
| `.pre-commit-config.yaml` | Configuration for pre-commit hooks. | Automates checks before committing code. | ✅ Yes | N/A | Keep | Essential for code quality and security. | |
| `.secrets.baseline` | Baseline for `detect-secrets`. | Prevents committing new secrets. | ✅ Yes | N/A | Keep | Essential for security. | |
| `AGENTS.md` | The primary guide for AI agents. | Provides instructions for AI agents. | ✅ Yes | N/A | Keep | The single source of truth for agent operations. | |
| `CHANGELOG.md` | Project changelog. | Tracks changes to the project. | ✅ Yes | N/A | Keep | Important for users and contributors. | |
| `CONTRIBUTING.md` | Contribution guidelines. | Provides instructions for human contributors. | ✅ Yes | N/A | Keep | Important for new contributors. | |
| `COVENANT.md` | Project covenant. | Defines the project's values and principles. | ✅ Yes | N/A | Keep | Important for project governance. | |
| `Dockerfile.gauntlet` | Dockerfile for the gauntlet build. | Creates a container for multi-compiler builds. | ✅ Yes | N/A | Keep | Essential for CI/CD. | |
| `file-matrix.md` | A file containing an analysis of the file hierarchy. | A temporary file for a one-time analysis. | ❌ No | N/A | Delete | This file is for a one-time analysis. | |
| `full-file-analysis.md` | The file you are currently reading. | A detailed analysis of the file hierarchy. | ❌ No | N/A | Keep (for now) | This file is the output of the current task. | |
| `LICENSE` | Project license. | Defines the legal terms of use for the project. | ✅ Yes | N/A | Keep | Essential for legal compliance. | |
| `Makefile` | Makefile for the project. | Provides a convenient interface for building and testing. | ✅ Yes | N/A | Keep | Simplifies the development workflow. | |
| `meson_options.txt` | Options for the Meson build system. | Configures the build process. | ✅ Yes | N/A | Keep | Essential for the build system. | |
| `meson.build` | Main build file for the Meson build system. | Defines the build process for the project. | ✅ Yes | N/A | Keep | The core of the build system. | |
| `NOTICE` | Project notice. | Provides legal notices. | ✅ Yes | N/A | Keep | Important for legal compliance. | |
| `README.md` | Project README. | The main entry point for the project. | ✅ Yes | N/A | Keep | The first file a new user or contributor will see. | |
| `run_gauntlet.sh` | Script to run the gauntlet build. | A helper script for the CI/CD process. | ✅ Yes | N/A | Keep | Part of the CI/CD process. | |
| `run-all-hooks.sh` | Script to run all git hooks. | A helper script for the development workflow. | ✅ Yes | N/A | Keep | Part of the development workflow. | |
| `run-github-actions-locally.sh` | Script to run GitHub Actions locally. | A helper script for the development workflow. | ✅ Yes | N/A | Keep | Part of the development workflow. | |

## Directory Analysis: `.ci/`

| Filepath | What? | Why? | Critical? | Hexagonal Architecture OK? | Recommended Action | Remarks | Notes |
|---|---|---|---|---|---|---|---|
| `.ci/.dockerignore` | Specifies files to ignore in the CI Docker build. | Reduces the Docker build context size. | ✅ Yes | N/A | Keep | Essential for an efficient CI build process. | |
| `.ci/Dockerfile` | Dockerfile for the CI environment. | Defines the CI environment with all necessary dependencies. | ✅ Yes | N/A | Keep | The foundation of the CI/CD pipeline. | |

## Directory Analysis: `.github/`

| Filepath | What? | Why? | Critical? | Hexagonal Architecture OK? | Recommended Action | Remarks | Notes |
|---|---|---|---|---|---|---|---|
| `.github/CODE_OF_CONDUCT.md` | Code of Conduct for the project. | Sets the standard for community interaction. | ✅ Yes | N/A | Keep | Important for a healthy community. | |
| `.github/CONTRIBUTING.md` | Contribution guidelines. | Provides instructions for contributors. | ✅ Yes | N/A | Keep | Essential for new contributors. | |
| `.github/early-testers.md` | Information about the early testers program. | To attract and inform early testers. | ❌ No | N/A | Keep | Good for community building. | |
| `.github/ETHICAL_USE.md` | Ethical use policy for the project. | Defines the intended use of the project. | ✅ Yes | N/A | Keep | Important for project philosophy. | |
| `.github/ISSUE_TEMPLATE/bug_report.md` | Template for bug reports. | To standardize bug reports. | ✅ Yes | N/A | Keep | Important for issue management. | |
| `.github/ISSUE_TEMPLATE/documentation.md` | Template for documentation improvements. | To standardize documentation requests. | ✅ Yes | N/A | Keep | Important for issue management. | |
| `.github/ISSUE_TEMPLATE/feature_request.md` | Template for feature requests. | To standardize feature requests. | ✅ Yes | N/A | Keep | Important for issue management. | |
| `.github/labeler.yml` | Configuration for the GitHub labeler action. | To automatically label pull requests. | ✅ Yes | N/A | Keep | Important for PR management. | |
| `.github/pull_request_template.md` | Template for pull requests. | To standardize pull request descriptions. | ✅ Yes | N/A | Keep | Important for PR management. | |
| `.github/SECURITY.md` | Security policy for the project. | To define how to report security vulnerabilities. | ✅ Yes | N/A | Keep | Essential for project security. | |
| `.github/workflows/apply-feedback.yml` | GitHub Actions workflow to apply feedback from PR comments. | Automates the process of applying feedback. | ✅ Yes | N/A | Keep | Part of the automated review process. | |
| `.github/workflows/auto-seed-review.yml` | GitHub Actions workflow to auto-seed review documents. | Automates the creation of review documents. | ✅ Yes | N/A | Keep | Part of the automated review process. | |
| `.github/workflows/c_core.yml` | GitHub Actions workflow for C core quality checks. | Runs linters and tests on the C code. | ✅ Yes | N/A | Keep | Essential for CI/CD. | |
| `.github/workflows/ci.yml` | Main GitHub Actions CI workflow. | The main CI workflow for the project. | ✅ Yes | N/A | Keep | Essential for CI/CD. Contains a migration notice. | |
| `.github/workflows/code-quality.yml` | GitHub Actions workflow for code quality checks. | Runs code quality checks. | ✅ Yes | N/A | Keep | Essential for CI/CD. Contains a migration notice. | |
| `.github/workflows/coderabbit-status.yml` | GitHub Actions workflow to check CodeRabbit status. | Checks the status of CodeRabbit reviews. | ✅ Yes | N/A | Keep | Part of the automated review process. | |
| `.github/workflows/core-quality.yml` | GitHub Actions workflow for core quality checks. | Runs a comprehensive set of quality checks on the core library. | ✅ Yes | N/A | Keep | Essential for CI/CD. | |
| `.github/workflows/gauntlet.yml` | GitHub Actions workflow for the gauntlet build. | Runs the multi-compiler gauntlet build. | ✅ Yes | N/A | Keep | Essential for CI/CD. | |
| `.github/workflows/release.yml` | GitHub Actions workflow for releases. | Automates the release process. | ✅ Yes | N/A | Keep | Essential for releases. Contains a migration notice. | |
| `.github/workflows/review-artifact-cleanup.yml` | GitHub Actions workflow to clean up review artifacts. | Automates the cleanup of review artifacts after a PR is merged. | ✅ Yes | N/A | Keep | Part of the automated review process. | |
| `.github/workflows/seed-review.yml` | GitHub Actions workflow to manually seed review documents. | Allows for manual creation of review documents. | ✅ Yes | N/A | Keep | Part of the automated review process. | |
| `.github/workflows/update-features-ledger.yml` | GitHub Actions workflow to update the features ledger. | Automates the update of the features ledger. | ✅ Yes | N/A | Keep | Part of the project management automation. | |

## Directory Analysis: `.legacy/`

| Filepath | What? | Why? | Critical? | Hexagonal Architecture OK? | Recommended Action | Remarks | Notes |
|---|---|---|---|---|---|---|---|
| `.legacy/` | A directory containing old code, documentation, and plans. | To preserve the history of the project. | ❌ No | ❌ No | Keep | This directory is a historical archive and should not be modified. | The contents of this directory are not relevant to the current state of the project, but are kept for posterity. |

## Directory Analysis: `.obsidian/`

| Filepath | What? | Why? | Critical? | Hexagonal Architecture OK? | Recommended Action | Remarks | Notes |
|---|---|---|---|---|---|---|---|
| `.obsidian/` | Configuration directory for the Obsidian note-taking app. | User-specific settings for personal note-taking. | ❌ No | N/A | Keep | This directory is properly ignored by Git. | This is a user-specific directory and not part of the project itself. |

## Directory Analysis: `apps/`

| Filepath | What? | Why? | Critical? | Hexagonal Architecture OK? | Recommended Action | Remarks | Notes |
|---|---|---|---|---|---|---|---|
| `apps/cli/cache_rebuild.c` | The `cache-rebuild` command for the CLI. | Implements the `cache-rebuild` command. | ✅ Yes | ⚠️ **In Progress** | Refactor | This is a driving adapter that should be refactored to use the hexagonal architecture. | |
| `apps/cli/cli_runtime.c` | CLI runtime context. | Provides a runtime context for the CLI. | ✅ Yes | ✅ Yes | Keep | This is a good example of a driving adapter. | |
| `apps/cli/cli_runtime.h` | Header for the CLI runtime context. | Defines the CLI runtime context. | ✅ Yes | ✅ Yes | Keep |  | |
| `apps/cli/install_hooks.c` | The `install-hooks` command for the CLI. | Implements the `install-hooks` command. | ✅ Yes | ⚠️ **In Progress** | Refactor | This is a driving adapter that should be refactored to use the hexagonal architecture. | |
| `apps/cli/link.c` | The `link` command for the CLI. | Implements the `link` command. | ✅ Yes | ⚠️ **In Progress** | Refactor | This is a driving adapter that should be refactored to use the hexagonal architecture. | |
| `apps/cli/link.md` | Documentation for the `link` command. | To document the `link` command. | ❌ No | N/A | Keep |  | |
| `apps/cli/list.c` | The `list` command for the CLI. | Implements the `list` command. | ✅ Yes | ⚠️ **In Progress** | Refactor | This is a driving adapter that should be refactored to use the hexagonal architecture. | |
| `apps/cli/list.md` | Documentation for the `list` command. | To document the `list` command. | ❌ No | N/A | Keep |  | |
| `apps/cli/main.c` | The main entry point for the CLI. | The main entry point for the CLI. | ✅ Yes | ✅ Yes | Keep | This is the composition root for the CLI application. | |
| `apps/cli/main.md` | Documentation for the `main` command. | To document the `main` command. | ❌ No | N/A | Keep |  | |
| `apps/cli/output.c` | Output functions for the CLI. | Provides functions for printing output to the console. | ✅ Yes | ✅ Yes | Keep | This is a good example of a driven adapter. | |

## Directory Analysis: `assets/`

| Filepath | What? | Why? | Critical? | Hexagonal Architecture OK? | Recommended Action | Remarks | Notes |
|---|---|---|---|---|---|---|---|
| `assets/` | A directory containing images and other assets. | For use in documentation and branding. | ❌ No | N/A | Keep | Important for the project's visual identity. | |

## Directory Analysis: `benchmarks/`

| Filepath | What? | Why? | Critical? | Hexagonal Architecture OK? | Recommended Action | Remarks | Notes |
|---|---|---|---|---|---|---|---|
| `benchmarks/README.md` | Documentation for the benchmarks. | To explain the purpose and usage of the benchmarks. | ❌ No | N/A | Keep |  | |
| `benchmarks/wildebeest_stampede.c` | A benchmark test for the cache. | To measure the performance of the cache. | ❌ No | ⚠️ **In Progress** | Refactor | This benchmark should be updated to use the hexagonal architecture. | It currently has direct dependencies on `libgit2`. |

## Directory Analysis: `core/`

### `core/include`

| Filepath | What? | Why? | Critical? | Hexagonal Architecture OK? | Recommended Action | Remarks | Notes |
|---|---|---|---|---|---|---|---|
| `core/include/gitmind/adapters/fs/posix_temp_adapter.h` | Header for the POSIX filesystem temp adapter. | Declares the factory function for the POSIX temp adapter. | ✅ Yes | ✅ Yes | Keep | This is a good example of an adapter header. | |
| `core/include/gitmind/adapters/git/libgit2_repository_port.h` | Header for the libgit2 repository port adapter. | Declares the factory function for the libgit2 adapter. | ✅ Yes | ✅ Yes | Keep | This is a good example of an adapter header. | |
| `core/include/gitmind/attribution.h` | Attribution metadata for edge tracking. | Defines the data structure for attribution. | ✅ Yes | ✅ Yes | Keep | This is a core domain model header. | |
| `core/include/gitmind/attribution/internal/defaults.h` | Internal header for attribution defaults. | Declares internal functions for attribution defaults. | ✅ Yes | ✅ Yes | Keep |  | |
| `core/include/gitmind/attribution/internal/env_loader.h` | Internal header for loading attribution from the environment. | Declares internal functions for loading attribution. | ✅ Yes | ✅ Yes | Keep |  | |
| `core/include/gitmind/cache.h` | High-performance query cache for git-mind edge data. | Defines the public API for the cache. | ✅ Yes | ⚠️ **In Progress** | Refactor | This header exposes implementation details and should be refactored to be a port. | |
| `core/include/gitmind/cache/bitmap.h` | Roaring Bitmap wrapper. | Provides a strongly-typed wrapper around Roaring Bitmaps. | ✅ Yes | ✅ Yes | Keep | A good example of a core domain utility. | |
| `core/include/gitmind/cache/cache.h` | Cache-related data structures and functions. | Defines data structures and functions for the cache. | ✅ Yes | ⚠️ **In Progress** | Refactor | This header exposes implementation details and should be refactored to be a port. | |
| `core/include/gitmind/cache/internal/edge_map.h` | Internal header for the edge map. | Declares internal functions for the edge map. | ✅ Yes | ✅ Yes | Keep |  | |
| `core/include/gitmind/cache/internal/rebuild_service.h` | Internal header for the cache rebuild service. | Declares internal functions for the cache rebuild service. | ✅ Yes | ✅ Yes | Keep |  | |
| `core/include/gitmind/cbor/cbor.h` | CBOR serialization and deserialization functions. | Provides functions for working with CBOR. | ✅ Yes | ✅ Yes | Keep | A core domain utility. | |
| `core/include/gitmind/cbor/keys.h` | CBOR keys for edge encodings. | Defines the keys used in CBOR-encoded edges. | ✅ Yes | ✅ Yes | Keep |  | |
| `core/include/gitmind/context.h` | Context structure for dependency injection. | Defines the context structure for dependency injection. | ✅ Yes | ✅ Yes | Keep | The core of the hexagonal architecture implementation. | |
| `core/include/gitmind/crypto/backend.h` | Crypto backend interface for dependency injection. | Defines the crypto backend interface. | ✅ Yes | ✅ Yes | Keep | A good example of a port definition. | |
| `core/include/gitmind/crypto/random.h` | Cryptographically secure random number generation. | Provides functions for generating random numbers. | ✅ Yes | ✅ Yes | Keep | A core domain utility. | |
| `core/include/gitmind/crypto/sha256.h` | SHA-256 hashing functions. | Provides functions for SHA-256 hashing. | ✅ Yes | ✅ Yes | Keep | A core domain utility. | |
| `core/include/gitmind/edge.h` | Edge operations for the git-mind knowledge graph. | Defines the public API for edges. | ✅ Yes | ✅ Yes | Keep | This is a core domain model header. | |
| `core/include/gitmind/edge/internal/blob_identity.h` | Internal header for resolving blob identity. | Declares internal functions for resolving blob identity. | ✅ Yes | ✅ Yes | Keep |  | |
| `core/include/gitmind/edge_attributed.h` | Attributed edge operations for the git-mind knowledge graph. | Defines the public API for attributed edges. | ✅ Yes | ✅ Yes | Keep | This is a core domain model header. | |
| `core/include/gitmind/error.h` | Error handling with chaining support and SSO. | Defines the error handling mechanism. | ✅ Yes | ✅ Yes | Keep | A core domain utility. | |
| `core/include/gitmind/fs/path_utils.h` | Filesystem path utilities. | Provides utility functions for working with paths. | ✅ Yes | ✅ Yes | Keep | A core domain utility. | |
| `core/include/gitmind/hooks/augment.h` | Git hook augmentation logic. | Defines the public API for hook augmentation. | ✅ Yes | ⚠️ **In Progress** | Refactor | This header exposes implementation details and should be refactored to be a port. | |
| `core/include/gitmind/io/io.h` | I/O operations interface with Result types. | Defines the I/O operations interface. | ✅ Yes | ✅ Yes | Keep | A good example of a port definition. | |
| `core/include/gitmind/journal.h` | Journal operations for reading and writing edges to Git storage. | Defines the public API for the journal. | ✅ Yes | ⚠️ **In Progress** | Refactor | This header exposes implementation details and should be refactored to be a port. | |
| `core/include/gitmind/portability/threads.h` | Portable threads.h implementation. | Provides a portable implementation of `threads.h`. | ✅ Yes | N/A | Keep | Important for cross-platform compatibility. | |
| `core/include/gitmind/ports/env_port.h` | Environment port. | Defines the port for accessing environment variables. | ✅ Yes | ✅ Yes | Keep | A good example of a port definition. | |
| `core/include/gitmind/ports/fs_temp_port.h` | Filesystem temp port. | Defines the port for filesystem temp operations. | ✅ Yes | ✅ Yes | Keep | A good example of a port definition. | |
| `core/include/gitmind/ports/git_repository_port.h` | Git repository port. | Defines the port for Git repository operations. | ✅ Yes | ✅ Yes | Keep | A good example of a port definition. | |
| `core/include/gitmind/result.h` | Result type for error handling. | Defines the `Result` type for error handling. | ✅ Yes | ✅ Yes | Keep | A core domain utility. | |
| `core/include/gitmind/security/memory.h` | Safe memory operations. | Provides safe memory operations. | ✅ Yes | ✅ Yes | Keep | A core domain utility. | |
| `core/include/gitmind/security/string.h` | Safe string operations. | Provides safe string operations. | ✅ Yes | ✅ Yes | Keep | A core domain utility. | |
| `core/include/gitmind/time/time.h` | Time operations interface with Result types. | Defines the time operations interface. | ✅ Yes | ✅ Yes | Keep | A good example of a port definition. | |
| `core/include/gitmind/types.h` | Core type definitions for git-mind. | Defines the core types for the project. | ✅ Yes | ✅ Yes | Keep | This is a core domain model header. | |
| `core/include/gitmind/types/id.h` | 256-bit identifier (SHA-256 based). | Defines the ID types for the project. | ✅ Yes | ✅ Yes | Keep | This is a core domain model header. | |
| `core/include/gitmind/types/path.h` | Path with validation state. | Defines the path type for the project. | ✅ Yes | ✅ Yes | Keep | This is a core domain model header. | |
| `core/include/gitmind/types/path_internal.h` | Internal header for path types. | Declares internal types for paths. | ✅ Yes | ✅ Yes | Keep |  | |
| `core/include/gitmind/types/string.h` | Owned string with explicit length and capacity. | Defines the string type for the project. | ✅ Yes | ✅ Yes | Keep | This is a core domain model header. | |
| `core/include/gitmind/types/ulid.h` | ULID generation and validation. | Provides functions for working with ULIDs. | ✅ Yes | ✅ Yes | Keep | A core domain utility. | |
| `core/include/gitmind/utf8/validate.h` | UTF-8 validation functions. | Provides functions for validating UTF-8 strings. | ✅ Yes | ✅ Yes | Keep | A core domain utility. | |
| `core/include/gitmind/util/memory.h` | Memory utility functions. | Provides memory utility functions. | ✅ Yes | ✅ Yes | Keep | A core domain utility. | |
| `core/include/gitmind/util/oid.h` | OID utility functions. | Provides utility functions for working with OIDs. | ✅ Yes | ✅ Yes | Keep | A core domain utility. | |
| `core/include/gitmind/util/ref.h` | Git ref utility functions. | Provides utility functions for working with Git refs. | ✅ Yes | ✅ Yes | Keep | A core domain utility. | |
| `core/include/gitmind/util/span.h` | Span utility functions. | Provides utility functions for working with spans. | ✅ Yes | ✅ Yes | Keep | A core domain utility. | |

### `core/src`

| Filepath | What? | Why? | Critical? | Hexagonal Architecture OK? | Recommended Action | Remarks | Notes |
|---|---|---|---|---|---|---|---|
| `core/src/adapters/config/env_adapter.c` | Environment adapter. | Implements the environment port. | ✅ Yes | ✅ Yes | Keep | A good example of an adapter. | |
| `core/src/adapters/fs/posix_temp_adapter.c` | POSIX filesystem temp adapter. | Implements the filesystem temp port. | ✅ Yes | ✅ Yes | Keep | A good example of an adapter. | |
| `core/src/adapters/git/libgit2_repository_port.c` | libgit2 repository port adapter. | Implements the Git repository port using libgit2. | ✅ Yes | ✅ Yes | Keep | A good example of an adapter. | |
| `core/src/app/cache/cache_rebuild_service.c` | Cache rebuild service. | Implements the cache rebuild service. | ✅ Yes | ⚠️ **In Progress** | Refactor | This is an application service that should be refactored to use the hexagonal architecture. | |
| `core/src/attribution/attribution.c` | Attribution implementation. | Implements the attribution logic. | ✅ Yes | ✅ Yes | Keep | This is a core domain model. | |
| `core/src/cache/bitmap.c` | Roaring Bitmap implementation. | Implements the Roaring Bitmap wrapper. | ✅ Yes | ✅ Yes | Keep | A core domain utility. | |
| `core/src/cache/builder.c` | Cache builder implementation. | Implements the cache builder. | ✅ Yes | ⚠️ **In Progress** | Refactor | This is a core component that needs to be refactored to use the hexagonal architecture. | |
| `core/src/cache/query.c` | Cache query implementation. | Implements the cache query logic. | ✅ Yes | ⚠️ **In Progress** | Refactor | This is a core component that needs to be refactored to use the hexagonal architecture. | |
| `core/src/cbor/cbor.c` | CBOR implementation. | Implements the CBOR serialization and deserialization. | ✅ Yes | ✅ Yes | Keep | A core domain utility. | |
| `core/src/crypto/backend.c` | Crypto backend implementation. | Implements the crypto backend. | ✅ Yes | ✅ Yes | Keep | A good example of an adapter. | |
| `core/src/crypto/random.c` | Random number generation implementation. | Implements the random number generation. | ✅ Yes | ✅ Yes | Keep | A core domain utility. | |
| `core/src/crypto/sha256.c` | SHA-256 implementation. | Implements the SHA-256 hashing. | ✅ Yes | ✅ Yes | Keep | A core domain utility. | |
| `core/src/domain/attribution/defaults.c` | Attribution defaults implementation. | Implements the attribution defaults logic. | ✅ Yes | ✅ Yes | Keep | This is a core domain model. | |
| `core/src/domain/attribution/env_loader.c` | Attribution environment loader implementation. | Implements the logic for loading attribution from the environment. | ✅ Yes | ✅ Yes | Keep | This is a core domain model. | |
| `core/src/domain/cache/edge_map.c` | Edge map implementation. | Implements the edge map for the cache. | ✅ Yes | ✅ Yes | Keep | This is a core domain model. | |
| `core/src/edge/attributed.c` | Attributed edge implementation. | Implements the attributed edge logic. | ✅ Yes | ✅ Yes | Keep | This is a core domain model. | |
| `core/src/edge/edge.c` | Edge implementation. | Implements the edge logic. | ✅ Yes | ✅ Yes | Keep | This is a core domain model. | |
| `core/src/edge/internal/blob_identity.c` | Blob identity implementation. | Implements the logic for resolving blob identity. | ✅ Yes | ✅ Yes | Keep |  | |
| `core/src/error/error.c` | Error handling implementation. | Implements the error handling mechanism. | ✅ Yes | ✅ Yes | Keep | A core domain utility. | |
| `core/src/fs/path_utils.c` | Filesystem path utilities implementation. | Implements the filesystem path utilities. | ✅ Yes | ✅ Yes | Keep | A core domain utility. | |
| `core/src/hooks/augment.c` | Git hook augmentation logic implementation. | Implements the Git hook augmentation logic. | ✅ Yes | ⚠️ **In Progress** | Refactor | This is a driving adapter that should be refactored to use the hexagonal architecture. | |
| `core/src/hooks/post-commit.c` | Post-commit hook implementation. | Implements the post-commit hook. | ✅ Yes | ⚠️ **In Progress** | Refactor | This is a driving adapter that should be refactored to use the hexagonal architecture. | |
| `core/src/io/io.c` | I/O operations implementation. | Implements the I/O operations interface. | ✅ Yes | ✅ Yes | Keep | A good example of an adapter. | |
| `core/src/journal/reader.c` | Journal reader implementation. | Implements the journal reader. | ✅ Yes | ⚠️ **In Progress** | Refactor | This is a core component that needs to be refactored to use the hexagonal architecture. | |
| `core/src/journal/writer.c` | Journal writer implementation. | Implements the journal writer. | ✅ Yes | ⚠️ **In Progress** | Refactor | This is a core component that needs to be refactored to use the hexagonal architecture. | |
| `core/src/ports/env_port.c` | Environment port implementation. | Implements the environment port. | ✅ Yes | ✅ Yes | Keep | A good example of an adapter. | |
| `core/src/time/time.c` | Time operations implementation. | Implements the time operations interface. | ✅ Yes | ✅ Yes | Keep | A good example of an adapter. | |
| `core/src/types/id.c` | ID implementation. | Implements the ID types. | ✅ Yes | ✅ Yes | Keep | This is a core domain model. | |
| `core/src/types/path.c` | Path implementation. | Implements the path type. | ✅ Yes | ✅ Yes | Keep | This is a core domain model. | |
| `core/src/types/string_core.c` | String core implementation. | Implements the core string logic. | ✅ Yes | ✅ Yes | Keep | A core domain utility. | |
| `core/src/types/string_utf8.c` | String UTF-8 implementation. | Implements the UTF-8 validation for strings. | ✅ Yes | ✅ Yes | Keep | A core domain utility. | |
| `core/src/types/string.c` | String implementation. | Implements the string type. | ✅ Yes | ✅ Yes | Keep | This is a core domain model. | |
| `core/src/types/ulid.c` | ULID implementation. | Implements the ULID generation and validation. | ✅ Yes | ✅ Yes | Keep | A core domain utility. | |
| `core/src/utf8/validate.c` | UTF-8 validation implementation. | Implements the UTF-8 validation. | ✅ Yes | ✅ Yes | Keep | A core domain utility. | |
| `core/src/util/oid.c` | OID utility implementation. | Implements the OID utility functions. | ✅ Yes | ✅ Yes | Keep | A core domain utility. | |
| `core/src/util/ref.c` | Git ref utility implementation. | Implements the Git ref utility functions. | ✅ Yes | ✅ Yes | Keep | A core domain utility. | |
