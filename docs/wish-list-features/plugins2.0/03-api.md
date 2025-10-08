---
title: API Specification: Plugin Architecture 2.0
description: Feature brainstorms and design sketches.
audience: [contributors]
domain: [project]
tags: [wishlist]
status: archive
last_updated: 2025-09-15
---

# API Specification: Plugin Architecture 2.0

## Summary

This document defines the __Plugin API v2__ for git-mind, specifying interfaces, data types, hook signatures, and available system calls across Native, WASM, and Script plugin runtimes.

---

## API Versioning

- All plugins must declare `api_version = "2.0"` in their manifest.
- Backward compatibility is not guaranteed between major versions.
- Compatibility is enforced at runtime before loading.

---

## Plugin Interface (`gm_plugin_v2_t`)

```c
typedef struct {
    uint32_t api_version;
    const char *name;
    const char *version;

    // Lifecycle
    int (*init)(gm_plugin_context_t *ctx);
    int (*shutdown)(void);

    // Hook functions
    int (*on_edge_created)(const gm_edge_t *edge);
    int (*on_node_accessed)(const gm_node_t *node);
    int (*on_commit_analyzed)(const gm_commit_t *commit);

    // Custom CLI commands
    const gm_command_t *commands;
    size_t command_count;

    // Optional WASM interface
    void *wasm_module;
    gm_wasm_imports_t wasm_imports;
} gm_plugin_v2_t;
```

---

## __Hook Definitions__

### `on_edge_created`

```
int on_edge_created(const gm_edge_t *edge);
```

Called when a new code relationship (edge) is detected.

---

### `on_node_accessed`

```
int on_node_accessed(const gm_node_t *node);
```

Triggered when a node is selected in the graph UI or referenced via API.

---

### `on_commit_analyzed`

```
int on_commit_analyzed(const gm_commit_t *commit);
```

Fires post-commit scan or Git sync.

---

## __CLI Commands__

### `gm_command_t`

```
typedef struct {
    const char *name;        // CLI name
    const char *description; // Help text
    int (*handler)(int argc, char **argv);
} gm_command_t;
```

---

## __Plugin Context:__

## `gm_plugin_context_t`

This is passed into init() and provides plugin-safe access to core services.

```
typedef struct {
    gm_log_fn log;
    gm_query_fn query;
    gm_emit_event_fn emit_event;
    gm_config_get_fn config_get;
    gm_add_edge_fn add_edge;
    gm_add_node_fn add_node;
} gm_plugin_context_t;
```

---

## __WASM Import API (Subset)__

Exposed to plugins running in WASM:

```
// Logging
extern "C" {
    fn gm_log(ptr: *const u8, len: usize);
}

// Data access
fn gm_get_node(id: u64) -> gm_node_t;
fn gm_get_edge(id: u64) -> gm_edge_t;

// Emit event
fn gm_emit_event(name_ptr: *const u8, name_len: usize);

// Create
fn gm_add_node(node_ptr: *const u8, node_len: usize);
fn gm_add_edge(edge_ptr: *const u8, edge_len: usize);
```

All strings are UTF-8, passed by pointer+length for safety.

---

## __TOML Manifest Schema__

```
name = "RefactorLinter"
version = "1.2.1"
api_version = "2.0"
entrypoint = "plugin.wasm"

permissions = ["nodes:read", "edges:write"]
signature = "sig-mindgpg-ABCDE123456"
```

---

## __Permissions Model__

|__Permission__|__Description__|
|---|---|
|`nodes:read`|Access to node metadata|
|`nodes:write`|Can create/update nodes|
|`edges:read`|Read graph relationships|
|`edges:write`|Modify graph relationships|
|`config:read`|Access project settings|
|`events:emit`|Broadcast plugin events|
|`log:write`|Emit log messages|

---

## __Plugin Status Introspection__

```
gm plugin list
```

Sample Output:

```
Plugin: RefactorLinter       ✅ Loaded
Type:   WASM
Hooks:  on_edge_created, on_commit_analyzed
Signed: Yes (trusted)
Version: 1.2.1
```

---

## __Fallbacks & Errors__

- If a plugin fails to load due to version or security issues, it is skipped and logged with reason.
- Plugins are isolated: one crashing plugin does not affect others.
