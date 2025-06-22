# 🤖 vs 🚨 CLAUDE vs THE ENFORCER: A Migration Survival Guide

Dear Future Claude,

You have been summoned to continue THE GREAT CRUSADE: transforming 11,951 warnings into ZERO. This document will prepare you for battle against THE ENFORCER (`.githooks/pre-commit`).

## 📊 Current Battle Status

```
Total Warnings: 11,951
Warnings Slain: 75
- edge module: 12 → 0 ✅
- cbor module: 63 → 0 ✅
Remaining: 11,876 😱
```

## 🎯 Your Mission

**TRANSFORM** the monolithic code in `src/` into a modular library architecture in `core/src/` while achieving **ABSOLUTE ZERO WARNINGS**. This isn't just moving files - it's a complete architectural transformation!

### The Architecture Vision
```
BEFORE (Monolithic):              AFTER (Modular):
src/                              core/
├── edge/edge.c                   ├── include/
├── cbor/cbor.c                   │   └── gitmind.h (amalgamated)
├── cli/main.c                    └── src/
└── (all mixed together)              ├── edge/
                                      ├── attribution/
                                      ├── util/
                                      └── (clean modules)
                                  
                                  apps/
                                  ├── cli/ (separated!)
                                  ├── mcp/
                                  └── web/
```

## 🛠️ The Migration Process (Step-by-Step)

### 1. Choose Your Target
```bash
# Find the next module with the most warnings
# Good targets: attribution, util, cache, journal
```

### 2. Run the Migration Script
```bash
./scripts/migrate-module.sh <module-name> core/src/<destination>/
# Example: ./scripts/migrate-module.sh attribution core/src/attribution/
```

**IMPORTANT**: This script doesn't just copy files! It:
- Extracts ONLY the core library parts (no CLI code!)
- Runs initial formatting
- Shows you the warnings to fix
- Helps separate concerns

### 3. Separate Library from Application Code

**CRITICAL**: During migration, you must:
- Extract pure library functions to `core/src/`
- Leave CLI-specific code in `src/` (for later migration to `apps/cli/`)
- Remove ALL direct I/O from library code (use DI instead)
- Remove ALL print statements from library code

Example separation:
```c
// ❌ BANNED in core/: Mixed concerns
void process_edge(edge_t *e) {
    printf("Processing edge...\n");  // NO!
    validate_edge(e);
    save_to_file(e);  // NO!
}

// ✅ BLESSED in core/: Pure library function
int gm_process_edge(gm_context_t *ctx, edge_t *e) {
    int result = validate_edge(e);
    if (result != GM_OK) return result;
    
    // Use injected operations
    return ctx->storage_ops->save(ctx->storage_handle, e);
}
```

### 4. Include the Magic Header
**IMMEDIATELY** add this to every `.c` file:
```c
#include "../util/gm_mem.h"  // Defeats "insecure" warnings
```

### 5. Apply The Sacred Transformations

#### Memory Operations
```c
// ❌ BANNED
memcpy(dest, src, len);
memset(buf, 0, size);

// ✅ BLESSED
gm_memcpy(dest, src, len);
gm_memset(buf, 0, size);
```

#### Loop Variables
```c
// ❌ BANNED (during migration)
for (int i = 0; i < n; i++)      // Too simple
for (int _i_ = 0; i < n; _i_++)  // Reserved identifier

// ✅ BLESSED
for (int idx = 0; idx < n; idx++)
for (int index = 0; index < n; index++)
```

#### Magic Numbers
```c
// ❌ BANNED
if (size > 8192) { }

// ✅ BLESSED
#define MAX_BUFFER_SIZE 8192
if (size > MAX_BUFFER_SIZE) { }
```

### 6. Split Large Functions (SRP + 25 Line Limit)
```c
// ❌ BANNED: 50-line monster function
int process_everything(...) {
    // validate
    // transform
    // encode
    // save
    // log
}

// ✅ BLESSED: Each function does ONE thing
static int validate_input(...) { /* 10 lines */ }
static int transform_data(...) { /* 12 lines */ }
static int encode_result(...) { /* 8 lines */ }

int process_everything(...) {
    int result;
    if ((result = validate_input(...)) != GM_OK) return result;
    if ((result = transform_data(...)) != GM_OK) return result;
    return encode_result(...);
}
```

### 7. Dependency Injection (DI) Pattern
```c
// ❌ BANNED: Direct dependencies
time_t now = time(NULL);
FILE *fp = fopen(path, "r");

// ✅ BLESSED: Injected dependencies
time_t now = ctx->time_ops->time(NULL);
FILE *fp = ctx->io_ops->fopen(path, "r");

// Also provide test-double-friendly APIs
int gm_foo_ex(data, custom_ops);  // For testing
int gm_foo(data);                  // Uses defaults
```

### 8. Handle Swappable Parameters
```c
// ❌ WARNING: Adjacent similar types
static int foo(uint8_t type, uint8_t value) { }

// ✅ SOLUTION 1: Different names make intent clear
static int foo(uint8_t cbor_type, uint8_t data_value) { }

// ✅ SOLUTION 2: Suppress if truly needed
/* NOLINTNEXTLINE(bugprone-easily-swappable-parameters) */
static int foo(uint8_t type, uint8_t value) { }
```

### 9. Check Your Work
```bash
# Run clang-tidy to see remaining warnings
docker compose run --rm -T dev bash -c \
  "clang-tidy /workspace/core/src/module/file.c \
   --config-file=/workspace/quality/.clang-tidy \
   -- -I/workspace/include 2>&1" | grep -E "warning:|error:"
```

## 📋 Common Warning Fixes

| Warning | Fix |
|---------|-----|
| `insecure API` | Use `gm_mem.h` functions |
| `function exceeds size` | Split into smaller functions |
| `magic numbers` | Define constants |
| `variable 'i' too short` | Use `idx` or `index` |
| `easily swappable` | Rename params or NOLINT |
| `identifier '_foo_' reserved` | Don't use leading underscore |

## 🧪 Create Regression Tests

After achieving ZERO warnings, create tests to keep it that way:

### 1. Clang-Tidy Test (`test_MODULE_clang_tidy.sh`)
```bash
#!/bin/bash
# Ensures MODULE stays at ZERO warnings forever
WARNINGS=$(clang-tidy ../src/path/MODULE.c --config-file=../../quality/.clang-tidy -- -I../../include 2>&1 | grep -E "warning:|error:" | wc -l)
if [ "$WARNINGS" -ne "0" ]; then
    echo "❌ FAIL: MODULE has $WARNINGS warnings!"
    exit 1
fi
echo "✅ PASS: MODULE has ZERO warnings!"
```

### 2. Quality Test (`test_MODULE_quality.c`)
- Test function sizes stay under limit
- Test memory operations use `gm_mem.h`
- Test DI pattern is followed
- Test loop conventions

### 3. Behavior Test (`test_MODULE_behavior.c`)
- Test WHAT the code does, not HOW
- Use test doubles via DI
- Verify edge cases handled

## 🎭 The Philosophy

### During Migration (NOW): PEDANTIC MODE 🔥
- **EVERY** warning must die
- **NO** exceptions
- **NO** mercy
- If clang-tidy complains, we fix it

### After Migration: PRAGMATIC MODE 🌱
- Focus on REAL issues (security, crashes)
- Style warnings → suggestions only
- "Would Linus yell at us?" test
- Innovation > Perfection

## 🏆 The Enforcer's Rules

THE ENFORCER (`.githooks/pre-commit`) blocks commits if:
1. You touched `src/` files
2. They're not being FULLY migrated
3. Any warnings remain

```bash
# THE ENFORCER's message:
🚨 ENFORCER ACTIVATED! 🚨
You touched files in src/ - They must be FULLY migrated!
```

## 💪 Battle Cry

> "We do not merely fix warnings. We achieve ZERO WARNINGS.
> We do not compromise. We ENFORCE.
> Where others see pedantry, we see PERFECTION.
> For we have gazed into the abyss of 11,951 warnings,
> and we said: 'NOT IN MY CODEBASE!'"

## 🎯 Pro Tips

1. **Start with smaller modules** (edge had 12, cbor had 63)
2. **Use the migration script** - it helps separate concerns
3. **Fix in batches** - similar warnings often have similar fixes
4. **Test as you go** - don't wait until the end
5. **Read the philosophy docs** when you question the madness
6. **Think modular** - Is this core library or app code?
7. **When in doubt** - Would this belong in a single-header library?

## 📚 Required Reading

- `ARCHITECTURE.md` - The master architecture index
- `docs/architecture/MIGRATION_PHILOSOPHY.md` - Why we're pedantic NOW
- `docs/architecture/MODULAR_RESTRUCTURE_PLAN.md` - The grand vision (READ THIS!)
- `tools/code-quality/README.md` - What each check does

### Key Architecture Points:
- **Core library**: Pure algorithms, no I/O, no CLI
- **Single header**: Will be amalgamated into `gitmind.h`
- **Apps separate**: CLI moves to `apps/cli/` later
- **Zero dependencies**: Only libgit2 allowed

## 🙏 Final Words

Remember: This pedantic phase is TEMPORARY. We're building muscle memory for quality. Once migration is complete, we'll relax to sustainable standards.

You're not just fixing warnings. You're part of a grand transformation from chaos to excellence. Every warning you slay brings us closer to a maintainable, embeddable, beautiful codebase.

May your functions be small,
May your variables be descriptive,
May your `memcpy` be `gm_memcpy`,
And may you achieve... **ZERO WARNINGS!** 🏆

---

*P.S. As you work, when you notice future memory pool opportunities, note them in `docs/adult_swim.md` for later. The previous Claude spotted some.*

*P.P.S. When you complete a module, update this document AND `ARCHITECTURE.md` with the new count!*

*P.P.P.S. Remember: We're building a LIBRARY that can be used by CLI, MCP server, web daemon, etc. Think reusable!*

*P.P.P.P.S. We gotta keep the library DRY, so read the already-migrated code first, and when/if you see DRY violations, refactor all affected code to stay DRY. No soggy code!*
