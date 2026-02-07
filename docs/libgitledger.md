# libgitledger: A Git-Native Ledger Library Specification

## 1. Overview

`libgitledger` is a portable, embeddable C library designed to create and manage secure, append-only ledgers directly within a Git repository. It provides a simple, powerful API for recording immutable, cryptographically-signed entries without requiring external databases or services.

The library aims to be the foundational engine for a wide range of applications, from deployment logging (`shiplog`) to knowledge management (`git-mind`), by offering a generic ledgering capability that is agnostic to the specific data being stored. It combines the robust architectural principles of `git-mind` with the mature feature set and real-world validation of `shiplog`.

## 2. Core Principles

*   **Git-Native:** Leverages Git's object model and reference system as the primary storage mechanism.
*   **Append-Only:** Ensures immutability and auditability through a linear, fast-forward-only commit history.
*   **Pluggable & Generic:** Decouples the library's core logic from specific data formats or indexing strategies.
*   **Secure & Auditable:** Integrates cryptographic signing, policy enforcement, and trust management.
*   **Robust & Portable:** Designed as a high-quality C library with a stable API, suitable for broad integration and language bindings.

## 3. Architectural Decisions

### 3.1. Git Interaction: `libgit2`

`libgitledger` will exclusively use **`libgit2`** for all Git operations, rather than shelling out to the `git` CLI.

*   **Justification:**
    *   **Robustness:** `libgit2` provides a stable, programmatic API, avoiding the fragility of parsing CLI output.
    *   **Performance:** In-process calls eliminate the overhead of process creation and inter-process communication.
    *   **Security:** Mitigates risks associated with shell command injection and PATH manipulation.
    *   **Portability:** As a C library, `libgit2` can be compiled and run on diverse platforms, which is crucial for `libgitledger`'s goal of broad embeddability and language bindings.

### 3.2. Internal Architecture: Hexagonal Model

The library will adhere to a **Hexagonal Architecture** (Ports & Adapters pattern), inspired by `git-mind`.

*   **Core/Domain:** Pure C modules containing the fundamental ledger logic, completely decoupled from Git or other external concerns.
*   **Ports:** Abstract interfaces (structs of function pointers) defining all necessary interactions with external systems (e.g., Git repository, filesystem, logging, memory allocation).
*   **Adapters:** Concrete implementations of these ports. For instance, a `libgit2`-based adapter will implement the `git_repository_port`.

### 3.3. Memory Management: Pluggable Allocator

`libgitledger` will provide a pluggable memory allocation interface, allowing users to supply their own `malloc`/`realloc`/`free` functions.

*   **Justification:**
    *   **Flexibility:** Accommodates environments with custom memory management (e.g., memory pools, specialized allocators, embedded systems).
    *   **Control:** Enables users to implement memory tracking, debugging, or specific memory safety policies.
    *   **Compatibility:** Prevents conflicts with host applications that use different C runtimes or allocators.
*   **API Impact:** A `gitledger_allocator_t` struct with function pointers and a `gitledger_set_allocator()` function will be part of the public API. A default implementation using standard `malloc`/`free` will be provided.

## 4. Feature Set

### 4.1. Core Ledger Operations

*   **Append-Only Ledger Core:**
    *   **Mechanism:** Each ledger entry is a Git commit on a dedicated ref (e.g., `refs/gitledger/journal/<ledger_name>`). The commit message contains the entry's payload.
    *   **Integrity:** Enforces fast-forward updates to maintain a linear, immutable history.
    *   **Named Ledger Instances:** Supports multiple distinct ledgers within a single repository, each identified by a unique `ledger_name` (generalizing `shiplog`'s "environment" concept).

*   **Cryptographic Signing:**
    *   **Mechanism:** Leverages Git's commit signing capabilities (GPG or SSH) to ensure the authenticity and integrity of ledger entries.
    *   **Integration:** The `gitledger_append()` function will automatically sign commits if configured and required by policy.

### 4.2. Data Handling & Customization

*   **Pluggable "Encoder" API (Custom Payloads):**
    *   **Concept:** The library is agnostic to the content format of ledger entries. Clients provide a callback function that generates the commit message body.
    *   **API Impact:** `gitledger_append()` will accept a `gitledger_encoder_fn` callback. This callback receives client-provided data and returns a `char*` containing the formatted commit message (e.g., CBOR for `git-mind`, JSON for `shiplog`).

*   **Git Note Attachments:**
    *   **Concept:** Allows attaching arbitrary, potentially large, user-defined data (e.g., command output, logs, build artifacts) to a specific ledger entry.
    *   **Mechanism:** Uses Git's notes feature, stored under a dedicated notes ref (e.g., `refs/gitledger/notes/<ledger_name>`).
    *   **API Impact:** `gitledger_attach_note(ledger_name, entry_sha, note_data)` function.

### 4.3. Policy & Trust Management

*   **Policy as Code:**
    *   **Concept:** The rules governing a ledger (e.g., `require_signed`, `allowed_authors`, structural requirements) are defined in a `policy.json` file, stored under a dedicated ref (e.g., `refs/gitledger/policy/<ledger_name>`).
    *   **API Impact:** Functions to read, update, and enforce this policy. The `gitledger_append()` function will consult the policy before committing.
    *   **Policy Query API:** Functions to programmatically inspect the active policy for a given ledger.

*   **Multi-Signature Trust:**
    *   **Concept:** Implements `shiplog`'s sophisticated trust model, allowing for N-of-M maintainer approvals for trust updates. The trust roster (`trust.json`) and allowed signers (`allowed_signers`) are stored under a dedicated ref (e.g., `refs/gitledger/trust/<ledger_name>`).
    *   **Signing Modes:** Supports both `chain` (co-signed commits) and `attestation` (detached SSH signatures).
    *   **API Impact:** Functions for managing the trust document and for verifying signatures against the trust policy.
    *   **Trust Query API:** Functions to programmatically inspect the active trust document for a given ledger.

### 4.4. Querying & Traversal

*   **Pluggable "Indexer" API with Roaring Bitmap Cache:**
    *   **Concept:** Provides high-performance querying by building a rebuildable cache of **Roaring Bitmaps**.
    *   **Mechanism:** A `gitledger_indexer_fn` callback allows clients to extract indexable terms from their custom payloads. `libgitledger` then builds and manages the bitmap indexes. The cache is stored in a dedicated ref (e.g., `refs/gitledger/cache/<ledger_name>`).
    *   **API Impact:** Functions for cache management (`gitledger_cache_rebuild()`) and powerful query operations (`gitledger_query_by_terms()`, `gitledger_query_and()`, `gitledger_query_or()`).

*   **Git Tag Association:**
    *   **Concept:** Links a ledger entry to a Git tag without the tag directly pointing to the ledger commit.
    *   **Mechanism:** Uses Git notes attached to the *tag object* itself, stored under a dedicated notes ref (e.g., `refs/gitledger/tag_notes`). The note content would reference the associated ledger entry's SHA.
    *   **API Impact:** `gitledger_tag_associate(ledger_name, tag_name, entry_sha)` function.

### 4.5. Robustness & Diagnostics

*   **Structured Error Reporting:**
    *   **Concept:** All API functions will return structured error objects (similar to `git-mind`'s `gm_result_t` and `gm_error_t`) containing error codes, messages, and optional cause chains.
    *   **API Impact:** `gitledger_result_t` and `gitledger_error_t` types.

*   **Pluggable Logging and Diagnostics:**
    *   **Concept:** Provides a pluggable interface for internal logging and diagnostic messages, allowing clients to integrate `libgitledger`'s output into their own logging systems.
    *   **API Impact:** `gitledger_logger_t` and `gitledger_set_logger()` functions.

*   **Ledger Integrity Verification:**
    *   **Concept:** A function to perform a comprehensive audit of a named ledger's integrity, checking fast-forward linearity, parent pointers, and (optionally) signature/policy compliance.
    *   **API Impact:** `gitledger_verify_ledger_integrity(ledger_name)` function.

## 5. Public API (High-Level Sketch)

The public API will be exposed through a main header file (e.g., `gitledger.h`) and will include functions for:

*   **Initialization & Configuration:** `gitledger_init()`, `gitledger_shutdown()`, `gitledger_set_allocator()`, `gitledger_set_logger()`.
*   **Ledger Management:** `gitledger_ledger_create()`, `gitledger_ledger_open()`, `gitledger_ledger_close()`.
*   **Entry Creation:** `gitledger_append()`.
*   **Entry Retrieval:** `gitledger_get_entry_by_sha()`, `gitledger_get_entry_by_seq()`, `gitledger_get_latest_entry()`.
*   **Querying:** `gitledger_query_by_terms()`, `gitledger_query_and()`, `gitledger_query_or()`, `gitledger_cache_rebuild()`.
*   **Policy & Trust:** `gitledger_policy_get()`, `gitledger_policy_update()`, `gitledger_trust_get()`, `gitledger_trust_update()`.
*   **Notes & Tags:** `gitledger_attach_note()`, `gitledger_get_note()`, `gitledger_tag_associate()`, `gitledger_tag_get_associated_entries()`.
*   **Verification:** `gitledger_verify_ledger_integrity()`.
*   **Error Handling:** Functions for inspecting `gitledger_error_t` objects.

## 6. Internal Workings (High-Level)

`libgitledger` will manage Git references and objects directly via its `libgit2` adapter. Each named ledger will correspond to a set of Git refs:
*   `refs/gitledger/journal/<ledger_name>`: The append-only commit chain.
*   `refs/gitledger/cache/<ledger_name>`: The rebuildable bitmap index.
*   `refs/gitledger/policy/<ledger_name>`: The policy document.
*   `refs/gitledger/trust/<ledger_name>`: The trust document and allowed signers.
*   `refs/gitledger/notes/<ledger_name>`: Git notes for entry attachments.
*   `refs/gitledger/tag_notes`: Git notes for tag associations.

All operations will flow through the hexagonal architecture, ensuring clear separation of concerns and testability.
