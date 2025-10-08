# File Hierarchy Analysis

This document provides an analysis of the files and directories at the root of the `git-mind` repository. The goal is to identify the purpose of each item, determine if it is properly ignored by Git, and recommend actions to clean up the repository.

| File/Folder | Origin & Purpose | Gitignored? | Recommended Action |
|---|---|---|---|
| **Directories** | | | |
| `.ci/` | Contains Dockerfile for CI environment. | ❌ | **Keep**. Essential for CI/CD. |
| `.claude/` | Legacy directory for a specific AI agent. | ✅ | **Delete**. This is legacy and is ignored. |
| `.crush/` | Log directory for the `crush` tool. | ❌ | **Add to .gitignore**. Log directories should not be tracked. |
| `.githooks/` | Contains git hooks for the project. | ❌ | **Keep**. Essential for development workflow. |
| `.github/` | Contains GitHub-specific files like workflows and issue templates. | ❌ | **Keep**. Essential for GitHub integration. |
| `.legacy/` | Contains legacy code and documentation. | ❌ | **Keep**. Preserved for historical context. |
| `.obsidian/` | Configuration for the Obsidian note-taking app. | ✅ | **Keep**. User-specific, and already ignored. |
| `.reuse/` | Contains licensing information for the REUSE tool. | ❌ | **Keep**. Important for license compliance. |
| `.trash/` | Contains deleted files. | ❌ | **Add to .gitignore and Delete**. This is a temporary directory and should not be tracked. |
| `.vscode/` | Contains VS Code settings for the project. | ✅ (partially) | **Keep**. The `.gitignore` is correctly configured to ignore user-specific settings. |
| `apps/` | Intended for future applications (CLI, etc.). | ❌ | **Keep**. Part of the project structure. |
| `archive/` | Contains archived planning documents. | ✅ | **Keep**. Ignored, but contains historical information. |
| `assets/` | Contains images and other assets for documentation. | ❌ | **Keep**. Part of the project documentation. |
| `benchmarks/` | Contains benchmark tests. | ❌ | **Keep**. Important for performance testing. |
| `build-debug/` | Build output directory. | ✅ | **Delete**. This is a build artifact and is correctly ignored. |
| `build-local/` | Build output directory. | ✅ | **Delete**. This is a build artifact and is correctly ignored. |
| `build-local2/` | Build output directory. | ✅ | **Delete**. This is a build artifact and is correctly ignored. |
| `docs/` | Contains project documentation. | ❌ | **Keep**. Essential for the project. |
| `include/` | Contains public C headers. | ❌ | **Keep**. Part of the C library. |
| `quality/` | Contains quality-related scripts and configurations. | ❌ | **Keep**. Part of the quality tooling. |
| `quality-reports/` | Contains reports from quality tools. | ✅ | **Delete**. These are generated artifacts and are correctly ignored. |
| `scripts/` | Contains various helper scripts. | ❌ | **Keep**. Important for the development workflow. |
| `tests/` | Contains integration and end-to-end tests. | ❌ | **Keep**. Essential for testing. |
| `tools/` | Contains various development tools. | ❌ | **Keep**. Important for the development workflow. |
| **Files** | | | |
| `.clang-format` | Configuration for the clang-format tool. | ❌ | **Keep**. Essential for code formatting. |
| `.coderabbit.yml` | Configuration for the CodeRabbit AI code reviewer. | ❌ | **Keep**. Important for code reviews. |
| `.dockerignore` | Specifies files to ignore in Docker builds. | ❌ | **Keep**. Essential for Docker builds. |
| `.env.example` | Example environment file. | ❌ | **Keep**. Provides a template for environment variables. |
| `.gitattributes` | Defines attributes for paths. | ❌ | **Keep**. Important for Git configuration. |
| `.gitignore` | Specifies files to be ignored by Git. | ❌ | **Keep**. Essential for Git configuration. |
| `.markdownlint.jsonc` | Configuration for the markdownlint tool. | ❌ | **Keep**. Essential for Markdown linting. |
| `.pre-commit-config.yaml` | Configuration for pre-commit hooks. | ❌ | **Keep**. Essential for the development workflow. |
| `.secrets.baseline` | Baseline for the detect-secrets tool. | ❌ | **Keep**. Important for security. |
| `AGENTS.md` | The primary guide for AI agents. | ❌ | **Keep**. The single source of truth for agent operations. |
| `CHANGELOG.md` | Project changelog. | ❌ | **Keep**. Important for tracking changes. |
| `CLAUDE.md` | Legacy guide for a specific AI agent. | ❌ | **Delete**. This file is deprecated and has been replaced by `AGENTS.md`. |
| `CONTRIBUTING.md` | Contribution guidelines. | ❌ | **Keep**. Important for contributors. |
| `COVENANT.md` | Project covenant. | ❌ | **Keep**. Important for project governance. |
| `Dockerfile.gauntlet` | Dockerfile for the gauntlet multi-compiler build. | ❌ | **Keep**. Essential for CI/CD. |
| `FACTS.md` | Temporary analysis file. | ❌ | **Delete**. This was a temporary file for analysis. |
| `LICENSE` | Project license. | ❌ | **Keep**. Important for legal reasons. |
| `Makefile` | Makefile for the project. | ❌ | **Keep**. Provides a convenient interface for building and testing. |
| `meson_options.txt` | Options for the Meson build system. | ❌ | **Keep**. Essential for the build system. |
| `meson.build` | Main build file for the Meson build system. | ❌ | **Keep**. Essential for the build system. |
| `NOTICE` | Project notice. | ❌ | **Keep**. Important for legal reasons. |
| `README.md` | Project README. | ❌ | **Keep**. The main entry point for the project. |
| `run_gauntlet.sh` | Script to run the gauntlet build. | ❌ | **Keep**. Part of the CI/CD process. |
| `run-all-hooks.sh` | Script to run all git hooks. | ❌ | **Keep**. Part of the development workflow. |
| `run-github-actions-locally.sh` | Script to run GitHub Actions locally. | ❌ | **Keep**. Part of the development workflow. |
