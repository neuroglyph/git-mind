---
title: 🚨 The Great Clang-Tidy Crusade
description: Tracking clang-tidy issues and progress toward zero warnings.
audience: [contributors, developers]
domain: [quality]
tags: [clang-tidy]
status: draft
last_updated: 2025-09-15
---

# 🚨 The Great Clang-Tidy Crusade

Table of Contents

- [Summary by Category](#-summary-by-category)
- [Critical Errors (Must Fix)](#-critical-errors-must-fix)
- [Warnings (Should Fix)](#-warnings-should-fix)
- [Phase 1 Complete: Easy Wins](#-phase-1-complete-easy-wins-2025-06-24)
- [Recommended Fix Order](#-recommended-fix-order-remaining-phases)
- [File Priority List](#-file-priority-list)
- [Success Metrics](#-success-metrics)
- [Suppression Guidelines](#-suppression-guidelines)

This document tracks all clang-tidy warnings/errors in the `core/` directory and provides actionable fixes for each category.

__Generated__: 2025-06-24  
__Last Updated__: 2025-06-24 (Phase 1 Complete)  
__Total Issues__: ~150 warnings/errors across 11 files  
__Fixed So Far__: ~65 warnings/errors ✅

## 📊 Summary by Category

| Category | Original | Fixed | Remaining | Severity | Difficulty |
|----------|---------|-------|-----------|----------|------------|
| Function Size/Complexity | 27 | 0 | 27 | ERROR | Medium |
| Short Variable Names | 45 | 45 ✅ | 0 | Warning | Easy |
| Global Variables | 8 | 0 | 8 | Warning | Medium |
| Security APIs | 16 | 0 | 16 | Warning | Hard |
| Naming Convention | 12 | 12 ✅ | 0 | ERROR | Easy |
| Swappable Parameters | 3 | 0 | 3 | ERROR | Medium |
| Missing Braces | 8 | 8 ✅ | 0 | Warning | Easy |
| Other | 31 | ~0 | 31 | Mixed | Mixed |

__Phase 1 Complete__: All easy wins executed! 65 warnings eliminated.

## 🔥 Critical Errors (Must Fix)

### 1. Function Size/Complexity Violations

__Files__: `error.c`, `path.c`, `string_core.c`, `utf8/validate.c`, `id.c`  
__Count__: 27 errors

__Examples__:

- `error.c:13`: `set_error_message` exceeds size/complexity
- `path.c:162`: `gm_path_new` exceeds size (50+ lines)
- `path.c:394`: `split_path_components` has cognitive complexity 12 (threshold 10)

__Fix Strategy__:

```c
// BEFORE: One giant function
static int process_everything(/* many params */) {
    // validate
    // parse
    // transform
    // encode
    // save
    // 100+ lines
}

// AFTER: Split into focused functions
static int validate_input(/* params */) { /* 15 lines */ }
static int parse_data(/* params */) { /* 20 lines */ }
static int transform_result(/* params */) { /* 15 lines */ }

static int process_everything(/* params */) {
    int result;
    if ((result = validate_input(...)) != 0) return result;
    if ((result = parse_data(...)) != 0) return result;
    return transform_result(...);
}
```

### 2. Naming Convention Violations

__Files__: `path.c`, `utf8/validate.c`  
__Count__: 12 errors

__Examples__:

- `path.c:13`: `path_sep_unix` should be `PATH_SEP_UNIX`
- `path.c:897`: `default_path_rules` should be `DEFAULT_PATH_RULES`
- Anonymous enum constants should be `GM_PATH_MAX_COMPONENTS`

__Fix__:

```c
// Constants must be UPPERCASE
#define PATH_SEP_UNIX '/'
#define PATH_SEP_WIN '\\'

// Global constants must be UPPERCASE
static const char* const SYMLINK_SUFFIX_MACOS = "@";

// Enum constants need prefix
enum {
    GM_PATH_MAX_COMPONENTS = 255,
    GM_PATH_MAX_LENGTH = 4096
};
```

### 3. Easily Swappable Parameters

__Files__: `string_core.c`, `utf8/validate.c`  
__Count__: 3 errors

__Examples__:

- `string_core.c:15`: `next_capacity(size_t current, size_t needed)`
- `utf8/validate.c:80`: `state_to_error(uint32_t state, uint32_t byte)`

__Fix Options__:

```c
// Option 1: Use different types
typedef struct { size_t value; } current_size_t;
typedef struct { size_t value; } needed_size_t;
static size_t next_capacity(current_size_t current, needed_size_t needed);

// Option 2: Use descriptive names + NOLINT
/* NOLINTNEXTLINE(bugprone-easily-swappable-parameters) */
static size_t next_capacity(size_t current_capacity, size_t needed_capacity);

// Option 3: Combine into struct
typedef struct {
    size_t current;
    size_t needed;
} capacity_calc_t;
static size_t next_capacity(capacity_calc_t calc);
```

### 4. Return Value Checks

__Files__: `error.c`  
__Count__: 4 errors

__Examples__:

- `error.c:34`: `vsnprintf` return value unchecked
- `error.c:248`: Function return value unused

__Fix__:

```c
// Check vsnprintf return value
int len = vsnprintf(buffer, size, fmt, args);
if (len < 0) {
    /* Handle encoding error */
    return GM_ERR_ENCODING;
}
if ((size_t)len >= size) {
    /* Handle truncation */
}
```

## ⚠️ Warnings (Should Fix)

### 1. Short Variable Names

__Files__: Almost all files  
__Count__: 45 warnings

__Common violations__:

- Single letters: `a`, `b`, `e`, `i`, `n`, `p`, `s`, `v`
- Two letters: `id`

__Fix__:

```c
// Parameters
bool gm_id_equal(gm_id_t id_a, gm_id_t id_b);  // not 'a', 'b'
static inline gm_err_result(gm_error_t* err);  // not 'e'

// Local variables
gm_string_t str = GM_UNWRAP(result);  // not 's'
const char* ptr = str->data;          // not 'p'
size_t idx = 0;                       // not 'i'
size_t count = list->length;          // not 'n'

// Exception: Loop indices can use 'i', 'j', 'k' if scope is small
for (size_t i = 0; i < count; i++) {  // OK if loop is < 10 lines
```

### 2. Non-Const Global Variables

__Files__: `backend.c`, `id.c`  
__Count__: 8 warnings

__Examples__:

- `backend.c:20`: `g_backend` pointer
- `id.c:25`: `g_siphash_key` array
- `backend.c:114`: `test_counter`

__Fix Strategy__:

```c
// Option 1: Make truly const if possible
static const gm_crypto_backend_t DEFAULT_BACKEND = { ... };

// Option 2: Use accessor functions for mutable globals
static gm_crypto_backend_t* g_backend = NULL;
static pthread_mutex_t g_backend_mutex = PTHREAD_MUTEX_INITIALIZER;

gm_crypto_backend_t* gm_crypto_get_backend(void) {
    pthread_mutex_lock(&g_backend_mutex);
    gm_crypto_backend_t* backend = g_backend;
    pthread_mutex_unlock(&g_backend_mutex);
    return backend;
}

// Option 3: Thread-local storage
static __thread uint32_t test_counter = 0;
```

### 3. Insecure API Warnings

__Files__: `error.c`, `id.c`  
__Count__: 16 warnings

__APIs flagged__:

- `vsnprintf` → wants `vsnprintf_s` (Annex K)
- `snprintf` → wants `snprintf_s`
- `sscanf` → wants `strtoul` or `sscanf_s`

__Fix Options__:

```c
// Option 1: Create safe wrappers (like we did for memcpy)
#ifdef __STDC_LIB_EXT1__
    #define GM_VSNPRINTF(dst, size, fmt, args) \
        vsnprintf_s((dst), (size), (fmt), (args))
#else
    // Add bounds checking wrapper
    static inline int gm_vsnprintf_safe(...) { ... }
    #define GM_VSNPRINTF gm_vsnprintf_safe
#endif

// Option 2: For sscanf → use strtoul
// BEFORE:
if (sscanf(hex + i * 2, "%2x", &byte) != 1) { ... }

// AFTER:
char hex_byte[3] = { hex[i*2], hex[i*2+1], '\0' };
char* endptr;
unsigned long val = strtoul(hex_byte, &endptr, 16);
if (endptr != hex_byte + 2 || val > 255) { /* error */ }
byte = (uint8_t)val;

// Option 3: Suppress with rationale
/* NOLINTNEXTLINE(clang-analyzer-security.insecureAPI) */
/* Rationale: Buffer size is compile-time constant and format is literal */
len = snprintf(buffer, sizeof(buffer), "%s:%d", file, line);
```

### 4. Missing Braces

__Files__: `string.c`  
__Count__: 8 warnings

__Fix__:

```c
// BEFORE
if (!a) return -1;
if (!b) return 1;

// AFTER
if (!a) {
    return -1;
}
if (!b) {
    return 1;
}
```

### 5. Integer Type Conversions

__Files__: `id.c`  
__Count__: 2 errors

__Example__: `id.c:112`: Multiplication result used as pointer offset

__Fix__:

```c
// BEFORE
out[i * 2] = hex[id.bytes[i] >> 4];

// AFTER
out[(size_t)i * 2] = hex[id.bytes[i] >> 4];
// Or better:
size_t offset = (size_t)i * 2;
out[offset] = hex[id.bytes[i] >> 4];
```

### 6. Cognitive Complexity

__Files__: `path.c`, `utf8/validate.c`  
__Count__: 6 warnings

__Fix__: Extract complex conditions into named functions:

```c
// BEFORE: Complex nested conditions
if (path && path[0] == '/' && (path[1] == '.' || 
    (path[1] == '/' && path[2] == '.')) && 
    !rules->allow_relative) {
    // ...
}

// AFTER: Clear intent
static bool is_relative_traversal(const char* path) {
    return path[0] == '/' && 
           (path[1] == '.' || 
            (path[1] == '/' && path[2] == '.'));
}

if (path && is_relative_traversal(path) && !rules->allow_relative) {
    // ...
}
```

## ✅ PHASE 1 COMPLETE: Easy Wins (2025-06-24)

### What Was Fixed

1. __Naming Convention Errors__ (12/12) ✅
   - `path.c`: PATH_SEP_UNIX, PATH_SEP_WIN, GM_PATH_MAX_*, SYMLINK_SUFFIX_MACOS, DEFAULT_PATH_RULES
   - `utf8/validate.c`: UTF8D array

2. __Short Variable Names__ (45/45) ✅
   - `error.c`: All `e` → `err`
   - `id.c`: `a,b` → `id_a,id_b`, `id` → `identifier`
   - `crypto/random.c`: `e` → `err`
   - Multiple helper functions fixed

3. __Missing Braces__ (8/8) ✅
   - `string.c`: All single-line if statements now have braces

4. __Integer Conversions__ (2/2) ✅
   - `id.c`: Fixed pointer arithmetic with explicit (size_t) casts

### Bonus Fixes Applied

- __Magic Numbers Eliminated__:
  - Defined HEX_CHARS_PER_BYTE, HIGH_NIBBLE_SHIFT, LOW_NIBBLE_MASK
  - Added TEST_HASH_LENGTH_HEADER_SIZE, BYTE_MASK, U32_HIGH_SHIFT in backend.c
- __Magic Strings Defined__:
  - HEX_DIGIT_STRING "0123456789abcdef"
  - HEX_FORMAT_2X "%2x"

## 🛠️ Recommended Fix Order (REMAINING PHASES)

1. __Phase 1: Easy Wins__ ✅ COMPLETE

2. __Phase 2: Function Refactoring__ (4-6 hours)
   - Split large functions into smaller ones
   - Reduce cognitive complexity
   - Each function should do ONE thing

3. __Phase 3: API Safety__ (2-3 hours)
   - Create safe wrappers for printf functions
   - Fix sscanf usage (use strtoul)
   - Add proper error checking

4. __Phase 4: Architecture__ (2-4 hours)
   - Address global variables
   - Fix swappable parameters
   - Consider thread safety

## 📋 File Priority List

Based on error count and severity:

1. __path.c__ - 27 errors (mostly function size)
2. __error.c__ - 10 errors (function size + unchecked returns)
3. __utf8/validate.c__ - 6 errors (naming + complexity)
4. __id.c__ - 5 errors (conversions + globals)
5. __string_core.c__ - 2 errors (swappable params + size)

## 🎯 Success Metrics

- [ ] Zero errors in clang-tidy output
- [ ] All functions < 50 lines
- [ ] All functions < cognitive complexity 10
- [ ] No raw global variables
- [ ] All variables >= 3 characters (except loop indices)
- [ ] All constants UPPERCASE
- [ ] All return values checked

## 🔧 Suppression Guidelines

Use `NOLINTNEXTLINE` sparingly and ONLY when:

1. The warning is a false positive
2. Fixing would make code significantly worse
3. You've added a comment explaining why

Example:

```c
/* NOLINTNEXTLINE(bugprone-easily-swappable-parameters) */
/* Rationale: Parameters are conceptually the same type but serve
   different semantic purposes clearly indicated by their names */
static int compare_versions(uint32_t version_old, uint32_t version_new)
```

---

Remember: These aren't just pedantic style issues. Each warning represents:

- Potential bugs (swappable parameters, unchecked returns)
- Maintenance debt (huge functions, bad names)
- Security issues (unsafe APIs, global state)

Fix them now while the codebase is small!

---

## 📜 INSTRUCTIONS FOR THE NEXT CLAUDE

### Current Status (2025-06-24)

- __Phase 1 COMPLETE__: All easy mechanical fixes done
- __Remaining__: ~85 warnings/errors requiring architectural changes

### Your Mission (Phase 2): Function Refactoring

Start with the worst offenders in `path.c`:

1. __Read these functions first__ (they're 50+ lines each):
   - `gm_path_new()` - line 162
   - `gm_path_join()` - line 213  
   - `split_path_components()` - line 394
   - `build_path_from_components()` - line 478
   - `gm_path_canonicalize()` - line 521

2. __Common refactoring patterns__:

   ```c
   // Extract validation logic
   static gm_result_void validate_path_safety(const char* path, rules) { }
   
   // Extract component processing
   static gm_result_void process_single_component(component, context) { }
   
   // Extract string building
   static gm_result_string build_path_string(components[], count) { }
   ```

3. __Gotchas to watch for__:
   - Don't break the safety checks (path traversal detection is critical!)
   - Keep error messages exactly the same
   - Run tests after EVERY function you split

### Tools at Your Disposal

- `make test-core` - Run all tests
- `docker compose run --rm -T dev bash -c "clang-tidy /workspace/core/src/types/path.c --config-file=/workspace/quality/.clang-tidy -- -I/workspace/core/include"` - Check specific file

### Critical Files Still Needing Work

1. __path.c__ - 27 function size errors (PRIORITY!)
2. __error.c__ - 10 errors (unchecked vsnprintf returns + large functions)
3. __utf8/validate.c__ - 6 errors (complex state machine)
4. __string_core.c__ - 2 errors (gm_string_trim too long)

### Phase 3 Preview (After Function Splits)

- Create `GM_SNPRINTF_SAFE`, `GM_VSNPRINTF_SAFE` wrappers
- Replace all sscanf with strtoul
- Check all return values

### Remember

- The Enforcer is watching - no new warnings allowed!
- Tests MUST pass after every change
- When in doubt, smaller functions are better
- Document any architectural decisions in TO_THE_NEXT_CLAUDE.md

Good luck, soldier. The codebase is counting on you! 🫡
