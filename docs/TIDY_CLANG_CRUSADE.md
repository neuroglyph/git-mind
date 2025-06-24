# 🚨 The Great Clang-Tidy Crusade

This document tracks all clang-tidy warnings/errors in the `core/` directory and provides actionable fixes for each category.

**Generated**: 2025-06-24  
**Last Updated**: 2025-06-24 (Phase 1 Complete)  
**Total Issues**: ~150 warnings/errors across 11 files  
**Fixed So Far**: ~65 warnings/errors ✅

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

**Phase 1 Complete**: All easy wins executed! 65 warnings eliminated.

## 🔥 Critical Errors (Must Fix)

### 1. Function Size/Complexity Violations
**Files**: `error.c`, `path.c`, `string_core.c`, `utf8/validate.c`, `id.c`  
**Count**: 27 errors

**Examples**:
- `error.c:13`: `set_error_message` exceeds size/complexity
- `path.c:162`: `gm_path_new` exceeds size (50+ lines)
- `path.c:394`: `split_path_components` has cognitive complexity 12 (threshold 10)

**Fix Strategy**:
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
**Files**: `path.c`, `utf8/validate.c`  
**Count**: 12 errors

**Examples**:
- `path.c:13`: `path_sep_unix` should be `PATH_SEP_UNIX`
- `path.c:897`: `default_path_rules` should be `DEFAULT_PATH_RULES`
- Anonymous enum constants should be `GM_PATH_MAX_COMPONENTS`

**Fix**:
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
**Files**: `string_core.c`, `utf8/validate.c`  
**Count**: 3 errors

**Examples**:
- `string_core.c:15`: `next_capacity(size_t current, size_t needed)`
- `utf8/validate.c:80`: `state_to_error(uint32_t state, uint32_t byte)`

**Fix Options**:
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
**Files**: `error.c`  
**Count**: 4 errors

**Examples**:
- `error.c:34`: `vsnprintf` return value unchecked
- `error.c:248`: Function return value unused

**Fix**:
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
**Files**: Almost all files  
**Count**: 45 warnings

**Common violations**:
- Single letters: `a`, `b`, `e`, `i`, `n`, `p`, `s`, `v`
- Two letters: `id`

**Fix**:
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
**Files**: `backend.c`, `id.c`  
**Count**: 8 warnings

**Examples**:
- `backend.c:20`: `g_backend` pointer
- `id.c:25`: `g_siphash_key` array
- `backend.c:114`: `test_counter`

**Fix Strategy**:
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
**Files**: `error.c`, `id.c`  
**Count**: 16 warnings

**APIs flagged**:
- `vsnprintf` → wants `vsnprintf_s` (Annex K)
- `snprintf` → wants `snprintf_s`
- `sscanf` → wants `strtoul` or `sscanf_s`

**Fix Options**:
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
**Files**: `string.c`  
**Count**: 8 warnings

**Fix**:
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
**Files**: `id.c`  
**Count**: 2 errors

**Example**: `id.c:112`: Multiplication result used as pointer offset

**Fix**:
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
**Files**: `path.c`, `utf8/validate.c`  
**Count**: 6 warnings

**Fix**: Extract complex conditions into named functions:
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

### What Was Fixed:
1. **Naming Convention Errors** (12/12) ✅
   - `path.c`: PATH_SEP_UNIX, PATH_SEP_WIN, GM_PATH_MAX_*, SYMLINK_SUFFIX_MACOS, DEFAULT_PATH_RULES
   - `utf8/validate.c`: UTF8D array
   
2. **Short Variable Names** (45/45) ✅
   - `error.c`: All `e` → `err`
   - `id.c`: `a,b` → `id_a,id_b`, `id` → `identifier` 
   - `crypto/random.c`: `e` → `err`
   - Multiple helper functions fixed

3. **Missing Braces** (8/8) ✅
   - `string.c`: All single-line if statements now have braces

4. **Integer Conversions** (2/2) ✅
   - `id.c`: Fixed pointer arithmetic with explicit (size_t) casts
   
### Bonus Fixes Applied:
- **Magic Numbers Eliminated**:
  - Defined HEX_CHARS_PER_BYTE, HIGH_NIBBLE_SHIFT, LOW_NIBBLE_MASK
  - Added TEST_HASH_LENGTH_HEADER_SIZE, BYTE_MASK, U32_HIGH_SHIFT in backend.c
- **Magic Strings Defined**:
  - HEX_DIGIT_STRING "0123456789abcdef"
  - HEX_FORMAT_2X "%2x"

## 🛠️ Recommended Fix Order (REMAINING PHASES)

1. **Phase 1: Easy Wins** ✅ COMPLETE

2. **Phase 2: Function Refactoring** (4-6 hours)
   - Split large functions into smaller ones
   - Reduce cognitive complexity
   - Each function should do ONE thing

3. **Phase 3: API Safety** (2-3 hours)
   - Create safe wrappers for printf functions
   - Fix sscanf usage (use strtoul)
   - Add proper error checking

4. **Phase 4: Architecture** (2-4 hours)
   - Address global variables
   - Fix swappable parameters
   - Consider thread safety

## 📋 File Priority List

Based on error count and severity:

1. **path.c** - 27 errors (mostly function size)
2. **error.c** - 10 errors (function size + unchecked returns)
3. **utf8/validate.c** - 6 errors (naming + complexity)
4. **id.c** - 5 errors (conversions + globals)
5. **string_core.c** - 2 errors (swappable params + size)

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
- **Phase 1 COMPLETE**: All easy mechanical fixes done
- **Remaining**: ~85 warnings/errors requiring architectural changes

### Your Mission (Phase 2): Function Refactoring
Start with the worst offenders in `path.c`:

1. **Read these functions first** (they're 50+ lines each):
   - `gm_path_new()` - line 162
   - `gm_path_join()` - line 213  
   - `split_path_components()` - line 394
   - `build_path_from_components()` - line 478
   - `gm_path_canonicalize()` - line 521

2. **Common refactoring patterns**:
   ```c
   // Extract validation logic
   static gm_result_void validate_path_safety(const char* path, rules) { }
   
   // Extract component processing
   static gm_result_void process_single_component(component, context) { }
   
   // Extract string building
   static gm_result_string build_path_string(components[], count) { }
   ```

3. **Gotchas to watch for**:
   - Don't break the safety checks (path traversal detection is critical!)
   - Keep error messages exactly the same
   - Run tests after EVERY function you split

### Tools at Your Disposal:
- `make test-core` - Run all tests
- `docker compose run --rm -T dev bash -c "clang-tidy /workspace/core/src/types/path.c --config-file=/workspace/quality/.clang-tidy -- -I/workspace/core/include"` - Check specific file

### Critical Files Still Needing Work:
1. **path.c** - 27 function size errors (PRIORITY!)
2. **error.c** - 10 errors (unchecked vsnprintf returns + large functions)
3. **utf8/validate.c** - 6 errors (complex state machine)
4. **string_core.c** - 2 errors (gm_string_trim too long)

### Phase 3 Preview (After Function Splits):
- Create `GM_SNPRINTF_SAFE`, `GM_VSNPRINTF_SAFE` wrappers
- Replace all sscanf with strtoul 
- Check all return values

### Remember:
- The Enforcer is watching - no new warnings allowed!
- Tests MUST pass after every change
- When in doubt, smaller functions are better
- Document any architectural decisions in TO_THE_NEXT_CLAUDE.md

Good luck, soldier. The codebase is counting on you! 🫡