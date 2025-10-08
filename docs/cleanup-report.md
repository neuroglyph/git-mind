# Documentation Cleanup Report

This report provides an analysis of the `docs/` directory and makes recommendations for its cleanup and reorganization.

## Overall Assessment

The `docs/` directory is currently in a state of disarray. It contains a mix of up-to-date, valuable information, and a significant amount of outdated, redundant, and contradictory historical documents. This makes it very difficult for a newcomer to understand the current state of the project and how to contribute.

The most significant problem is the presence of multiple, conflicting plans for the project's architecture and development. The `docs/enforcer` directory, in particular, represents a past, abandoned effort and should be archived.

## Recommendations for Newcomers

A newcomer should start by reading the following documents in this order:

1.  `README.md` (the main project README)
2.  `CONTRIBUTING.md`
3.  `docs/DEV_SETUP.md`
4.  `docs/philosophy.md`
5.  `docs/TECHNICAL.md`
6.  `docs/architecture/README.md` (the new hexagonal architecture overview)
7.  `docs/cli/README.md` (for an overview of the CLI)

## Files to Delete/Archive

The following files and directories are no longer relevant and should be deleted or moved to the `.legacy/` directory to reduce clutter and confusion.

*   **`docs/enforcer/`**: This entire directory is a relic of a past refactoring effort and is now obsolete. It should be moved to `.legacy/archive/enforcer`. 
*   **`docs/planning/`**: This directory contains a mix of outdated and current plans. The outdated plans should be moved to `.legacy/archive/planning`.
*   **`docs/PRDs/`**: Similar to `docs/planning`, this directory contains a mix of old and new. Outdated PRDs should be archived.
*   **`docs/specs/`**: This directory should be reviewed, and any outdated specifications should be archived.
*   **`docs/wish-list-features/`**: This directory contains brainstorming documents that are largely historical. They should be moved to `.legacy/archive/wish-list-features`.
*   **`docs/adult_swim.md`**: The purpose of this document is unclear, and it seems to be a collection of random notes. It should be reviewed and likely archived.
*   **`docs/TECHNICAL.md.md`**: This is a duplicate file and should be deleted.

## Files to Consolidate/Merge

*   **`docs/architecture/`**: This directory contains multiple, overlapping architecture documents. The following should be consolidated into a single, coherent set of documents under the new hexagonal architecture plan:
    *   `System_Architecture.md`
    *   `journal-architecture-pivot.md`
    *   `bitmap-cache-design.md`
    *   `MODULAR_RESTRUCTURE_PLAN.md`
*   **`docs/cli/`**: The individual markdown files for each CLI command should be consolidated into a single, comprehensive CLI reference guide.

## Files to Keep (and potentially update)

*   **`docs/README.md`**: This should be the main entry point for the documentation.
*   **`docs/architecture/README.md`**: This should be the single source of truth for the hexagonal architecture.
*   **`docs/cli/README.md`**: This should be the main entry point for the CLI documentation.
*   **`docs/DEV_SETUP.md`**: This is essential for new contributors.
*   **`docs/CI_STRATEGY.md`**: This is important for understanding the CI/CD process.
*   **`docs/philosophy.md`**: This provides important context for the project.
*   **`docs/TECHNICAL.md`**: This is a good high-level overview.
*   **`docs/tutorial.md`**: This is a good starting point for new users.

## Proposed New Structure

I recommend the following new directory structure for the `docs/` directory:

```
docs/
├── README.md           # Main entry point, with a clear reading path for different audiences
├── tutorial.md         # A quick-start guide for new users
├── user-guide/         # In-depth user documentation
│   ├── cli-reference.md
│   └── ...
├── contributor-guide/  # For developers and contributors
│   ├── dev-setup.md
│   ├── ci-strategy.md
│   └── ...
├── architecture/       # Architectural decision records and high-level design
│   ├── README.md         # Overview of the hexagonal architecture
│   ├── adr/              # Architectural Decision Records
│   └── ...
└── project/            # Project management and governance
    ├── philosophy.md
    ├── roadmap.md
    └── ...
```

This structure separates the documentation by audience and purpose, making it much easier to navigate and maintain.
