# 🚫 STOP! Read This Before Writing ANY Code

## 🔥 The Great Migration Context

We are fixing **11,951 compiler warnings** caused by years of:
- "TODO: implement later" 
- Magic numbers everywhere
- Placeholder implementations
- "Quick fixes" that became permanent
- Functions that "mostly work"

**YOUR MISSION**: Build foundations with ZERO warnings, ZERO shortcuts, ZERO excuses.

## ❌ FORBIDDEN: Things That Got Us Into This Mess

### 1. NO Placeholder Implementations
```c
// ❌ FORBIDDEN - This is how we got 11,951 warnings!
static void sha256(const void* data, size_t len, uint8_t out[32]) {
    /* TODO: Use real SHA256 implementation */
    memset(out, 0, 32);  // "temporary" for 5 years...
}

// ✅ REQUIRED - Implement it properly or don't ship it!
static int sha256(const void* data, size_t len, uint8_t out[32]) {
    if (!data || !out) return GM_ERR_INVALID_ARGUMENT;
    return git_hash_buf(out, data, len, GIT_HASH_SHA256);
}
```

### 2. NO Magic Numbers EVER
```c
// ❌ FORBIDDEN - Magic numbers killed this codebase!
typedef struct gm_id {
    uint8_t bytes[32];  // What's 32? SHA256? MD5? Random?
} gm_id_t;

void gm_id_to_hex(gm_id_t id, char out[65]);  // Why 65?

// ✅ REQUIRED - Define EVERYTHING!
#define GM_ID_SIZE 32           /* SHA-256 digest size in bytes */
#define GM_ID_HEX_SIZE 65       /* 32 bytes * 2 + null terminator */

typedef struct gm_id {
    uint8_t bytes[GM_ID_SIZE];
} gm_id_t;

void gm_id_to_hex(gm_id_t id, char out[GM_ID_HEX_SIZE]);
```

### 3. NO Fake Implementations
```c
// ❌ FORBIDDEN - "Works for testing" = doesn't work!
gm_id_t gm_id_generate(void) {
    static uint32_t counter = 0;
    counter++;  // "random enough for now"... NO!
}

// ✅ REQUIRED - Use proper implementations!
gm_result_id gm_id_generate(void) {
    gm_id_t id;
    if (RAND_bytes(id.bytes, GM_ID_SIZE) != 1) {
        return gm_err_id(GM_ERROR(GM_ERR_CRYPTO_FAILED, 
                                  "Failed to generate random ID"));
    }
    return gm_ok_id(id);
}
```

### 4. NO Functions Without Error Handling
```c
// ❌ FORBIDDEN - Can't report failure = will fail silently!
gm_id_t gm_id_from_data(const void* data, size_t len);

// ✅ REQUIRED - Everything can fail!
gm_result_id gm_id_from_data(const void* data, size_t len);
```

### 5. NO Incomplete Implementations
```c
// ❌ FORBIDDEN - Declared but not implemented = lies!
gm_result_string gm_string_trim(const gm_string_t* str);  
// Implementation: ¯\_(ツ)_/¯

// ✅ REQUIRED - Implement it NOW or remove from header!
```

## 📋 Migration Rules (NO EXCEPTIONS!)

### Starting ANY New File
1. **Read these first**:
   - `docs/enforcer/ROADMAP_TO_REFACTORING.md` - The plan
   - `TASKLIST.md` - Current status and TODOs
   - `TO_THE_NEXT_CLAUDE.md` - Handoff notes

2. **Every file MUST have**:
   ```c
   /* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
   /* © 2025 J. Kirby Ross / Neuroglyph Collective */
   ```

3. **Before writing ANY code**:
   - ❓ Can this function fail? → Return `gm_result_*`
   - 🔢 Any numbers? → `#define` them with descriptive names
   - 🧪 Where's the test? → Write test FIRST
   - 🐳 Build locally? → NO! Use `make test-core`

### Code Quality Checklist (EVERY Function)
- [ ] NO magic numbers (all constants defined)
- [ ] NO magic strings (all strings defined)
- [ ] NO TODO comments (implement it or delete it)
- [ ] NO placeholder code (real implementation only)
- [ ] NO missing error handling (use Result types)
- [ ] NO untested code (TDD - test first!)
- [ ] NO local builds (Docker only via make)
- [ ] NO warnings (not even one!)

## 🏗️ Current Architecture Rules

### core/ Directory (New Clean Code)
- **ZERO warnings policy** - Not negotiable
- **Everything in Docker** - `make test-core`
- **Result types everywhere** - Use `GM_RESULT_DEF`
- **No magic anything** - Define all constants
- **Complete implementations** - No placeholders

### src/ Directory (Legacy Quarantine)
- **DO NOT TOUCH** - 11,951 warnings
- **DO NOT MODIFY** - Enforcer will block you
- **DO NOT FIX** - Rewrite in core/ instead
- **DO NOT BUILD** - It's broken on purpose

## 🛠️ Required Development Flow

### 1. Check Current State
```bash
cat TASKLIST.md              # What needs doing?
cat TO_THE_NEXT_CLAUDE.md    # Any handoff notes?
```

### 2. Implement Feature
```bash
# 1. Write test first (TDD)
vim core/tests/unit/test_feature.c

# 2. Run test (it should fail)
make test-core

# 3. Implement feature
vim core/src/feature.c

# 4. Run test (it should pass)
make test-core

# 5. Check for warnings
make check-core
```

### 3. Common Commands
```bash
make test-core     # Run core/ tests in Docker
make dev          # Get Docker shell
make check        # Run quality checks
```

## 🚨 Error Handling Patterns

### ALWAYS Use Result Types
```c
// Define result type for your return value
GM_RESULT_DEF(gm_result_widget, widget_t*);

// Functions that can fail MUST return results
gm_result_widget create_widget(const char* name) {
    if (!name) {
        return gm_err_widget(GM_ERROR(GM_ERR_INVALID_ARGUMENT, 
                                      "Widget name cannot be NULL"));
    }
    
    widget_t* w = malloc(sizeof(widget_t));
    if (!w) {
        return gm_err_widget(GM_ERROR(GM_ERR_OUT_OF_MEMORY,
                                      "Failed to allocate widget"));
    }
    
    return gm_ok_widget(w);
}
```

### ALWAYS Check Results
```c
gm_result_widget result = create_widget("test");
if (GM_IS_ERR(result)) {
    gm_error_print(GM_UNWRAP_ERR(result));
    return -1;
}
widget_t* w = GM_UNWRAP(result);
```

## 📏 Constant Definition Rules

### For Every Number
```c
// ❌ NEVER
char buffer[256];
if (count > 15) { }

// ✅ ALWAYS
#define BUFFER_SIZE 256
#define MAX_ITEM_COUNT 15

char buffer[BUFFER_SIZE];
if (count > MAX_ITEM_COUNT) { }
```

### For Every String
```c
// ❌ NEVER
return GM_ERROR(code, "Operation failed");

// ✅ ALWAYS
#define ERR_MSG_OPERATION_FAILED "Operation failed"
return GM_ERROR(code, ERR_MSG_OPERATION_FAILED);
```

## 🧪 Testing Requirements

### Test-Driven Development (TDD)
1. **Write test first** - It will fail
2. **Write minimal code** - Make test pass
3. **Refactor** - Keep tests green
4. **NO untested code** - 100% coverage

### Test Naming
```c
// Test file: test_<module>.c
// Test function: test_<module>_<scenario>

void test_string_new_handles_null(void) {
    gm_result_string result = gm_string_new(NULL);
    assert(GM_IS_OK(result));  // Should create empty string
    // ...
}
```

## 🔥 The Enforcer Is Watching

Remember:
- Pre-commit hooks block changes to src/
- CI runs quality checks on all code
- One warning = build fails
- One TODO = code review fails

## 💀 Examples of What Created 11,951 Warnings

```c
// 1. Magic numbers everywhere
write(fd, buffer, 4096);  // What's 4096?

// 2. No error handling
void* ptr = malloc(size);  // What if it fails?
strcpy(dst, src);         // What if dst too small?

// 3. Placeholder code
// TODO: Implement this properly
return 0;  // Just return success for now...

// 4. Copy-paste programming
// Copied from StackOverflow, not sure how it works...

// 5. "Temporary" hacks
#ifdef QUICK_HACK  // Added in 2019, still here...
```

## ✅ Final Checklist Before ANY Push

- [ ] Read `TASKLIST.md` for current state
- [ ] All tests pass in Docker (`make test-core`)
- [ ] Zero compiler warnings
- [ ] No magic numbers or strings
- [ ] No TODO comments
- [ ] No placeholder implementations
- [ ] All functions have error handling
- [ ] All public functions have tests
- [ ] Code follows patterns in specs

## 🎯 Remember Your Mission

You're not just writing code. You're:
1. **Fixing 11,951 warnings** from bad habits
2. **Building foundations** for the next decade
3. **Setting the standard** for quality

Every shortcut you take today becomes someone else's nightmare tomorrow.

**Build it right, or don't build it at all.**

---
*The price of excellence is eternal vigilance against the temptation to write "good enough" code.*
## 🔥 Lessons Learned (2025-06-22 Session)

### 1. **Error Result Helpers Are Essential**
When defining custom result types with `GM_RESULT_DEF`, always create helper functions:
```c
/* Define the result type */
GM_RESULT_DEF(gm_result_string, gm_string_t);

/* ALWAYS add this helper */
static inline gm_result_string gm_err_string(gm_error_t* e) {
    return (gm_result_string){ .ok = false, .u.err = e };
}
```
Without these helpers, you'll get cryptic compiler errors about incompatible types.

### 2. **SipHash Requires Proper libsodium Constants**
When using SipHash-2-4, use the proper constants:
```c
/* ✅ CORRECT */
uint8_t key[crypto_shorthash_siphash24_KEYBYTES];
uint8_t out[crypto_shorthash_siphash24_BYTES];
crypto_shorthash_siphash24(out, data, len, key);

/* ❌ WRONG - leads to confusing errors */
uint64_t hash = crypto_shorthash_siphash24(data, len, key);
```

### 3. **SSO Implementation Requires va_list Copying**
When implementing SSO for error messages, you can't use va_list twice:
```c
/* ❌ WRONG - undefined behavior */
int len = vsnprintf(temp, sizeof(temp), fmt, args);
vsnprintf(final, len + 1, fmt, args);  /* args already consumed\! */

/* ✅ CORRECT - use va_copy */
va_list args_copy;
va_copy(args_copy, args);
int len = vsnprintf(temp, sizeof(temp), fmt, args);
vsnprintf(final, len + 1, fmt, args_copy);
va_end(args_copy);
```

### 4. **Always Suppress Unused Parameter Warnings**
In implementations where parameters are reserved for future use:
```c
void function(void* unused_param) {
    (void)unused_param;  /* Suppress warning */
    /* Implementation */
}
```

### 5. **Makefile Dependencies Are Transitive**
When modules depend on each other, ALL transitive dependencies must be linked:
```c
/* If path uses string, string uses crypto, crypto uses error... */
/* Then path tests need ALL of them: */
$(CC) test_path.c $(PATH_OBJS) $(STRING_OBJS) $(CRYPTO_OBJS) $(ERROR_OBJS)
```

### 6. **Test Helper Functions for Internal Access**
When testing modules with opaque types, create test-specific accessors:
```c
/* In test file only */
static const char* test_get_error_message(const gm_error_t* err) {
    return err->heap_alloc ? err->msg.heap : err->msg.small;
}
```

### 7. **Crypto DI Enables Deterministic Testing**
Always abstract crypto behind an interface:
- Production uses real crypto (libsodium)
- Tests use deterministic implementations
- Switch backends at runtime for different scenarios

## 🎖️ LIEUTENANT DEVOPS BATTLE ORDERS (2025-06-24)

### 1. Branch & Ticket Priority Queue

| Priority | Branch Name | Ticket | Objective | Status |
|----------|-------------|--------|-----------|---------|
| P0 | `hotfix/result-unification` | #HFX-001 | Purge duplicate gm_result_* typedefs from random.h | ✅ COMPLETE |
| P0 | `feat/id_context` | #FTR-002 | Finish thread-safe ID context; TSAN must run clean | ⏳ PENDING |
| P1 | `refactor/solid-fixes-01` | #RFC-003 | Convert remaining 12 void functions to gm_result_* | ⏳ PENDING |
| P1 | `refactor/path-modular` | #RFC-004 | Split path.c into 4 SRP modules | ⏳ PENDING |

**Rule**: Work one branch at a time; keep PRs laser-focused.

### 2. Baseline Reduction Strategy

**Current Baseline**: 202 warnings (as of 2025-06-24)

**Minimum reduction per PR**: -10 warnings

**Easy wins to target first**:
- Replace `vsnprintf` / `snprintf` / `sscanf` / `memset` with secure variants
- Extract helper functions to reduce cognitive complexity below 15
- Fix identifier naming (PATH_SEP_UNIX, DEFAULT_PATH_RULES, etc.)
- Fix `bugprone-easily-swappable-parameters` (often kills 3-4 lines)

### 3. CI/Pre-commit Discipline

- ✅ All hooks must pass before pushing
- ✅ CI must stay green; any new warning above baseline = auto-fail
- ✅ Update `tools/baseline.txt` ONLY by deleting lines (never add)
- ✅ No `.clang-tidy` or pipeline edits without Central Command approval

### 4. Commit & PR Protocol

1. **Branch from latest main**

2. **Follow commit style**:
   ```
   <type>(<scope>): <summary>
   
   e.g. fix(result): remove duplicate u64 typedef
   ```
3. **Open PR with**:
   - Link to ticket ID
   - Checklist:
     - [ ] ≥10 warnings removed
     - [ ] baseline.txt updated
     - [ ] CI green
4. **No merges until reviews pass**

### 5. Progress Timeline

- **Today EOD**: Open PR for `hotfix/result-unification`
- **Next 48h**: Merge hotfix and `feat/id_context`
- **End of week**: Knock baseline under 150 with `solid-fixes-01`

### 6. PR Progress Log

#### PR #127 - hotfix/result-unification (2025-06-24)
- **Status**: ✅ READY FOR PR
- **Warnings removed**: 0 (hotfix - no warning reduction)
- **Baseline before**: 202
- **Baseline after**: 202
- **Changes**:
  - Removed duplicate `gm_result_u64` typedef from random.h
  - Removed duplicate `gm_err_u32/u64` helpers from id.c, random.c, id_context.c
  - Fixed missing includes and undefined macros in id_context.c
  - Fixed thread safety test bugs (ID generation, buffer overflow)
- **Tests**: All passing ✅

---
*Lieutenant DevOps reporting. Updates posted after each PR merge.*