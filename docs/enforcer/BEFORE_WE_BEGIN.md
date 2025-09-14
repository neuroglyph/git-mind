# 🚧 BEFORE WE BEGIN: Clean Slate Strategy

## The Situation

We started migrating some modules from `src/` to `core/` but realized we need solid foundations first. Now we're going to:

1. __Move everything back to `src/`__ - Start with a completely empty `core/`
2. __Build foundations first__ - Implement our specs with zero technical debt
3. __Then migrate properly__ - Using the new foundations

## The Plan

### 1. Move Everything Back to `src/` (Clean Slate)

```bash
# Move partially migrated code back
mv core/src/attribution/* src/attribution/
mv core/src/edge/* src/edge/
mv core/src/util/* src/util/
mv core/tests/* tests/legacy/  # or back to tests/

# Remove the old structure
rm -rf core/*

# Create clean structure
mkdir -p core/{include/gitmind,src,tests}
mkdir -p core/include/gitmind/{types,security}
mkdir -p core/src/{error,types,security}
mkdir -p core/tests/{unit,integration,e2e}
```

### 2. Mental Model During Migration

```
src/     = "the old city" (quarantined, 11,000+ warnings, don't touch)
core/    = "the new city" (pristine, zero warnings from day one)
apps/    = "the suburbs" (CLI and other apps, built on new foundations)
```

### 3. Test Strategy

```bash
tests/
├── legacy/        # All current tests (test the old implementation)
│   └── ...        # May have warnings, may fail - that's OK
└── core/          # New tests for core/ code only
    ├── unit/      # Unit tests for new modules
    ├── integration/ # Integration tests
    └── e2e/       # End-to-end tests
```

__Key Points:__

- Legacy tests may fail after moving code back - __that's fine__
- We only care about `core/` tests passing
- CI should only run `core/` tests
- Once a module is reimplemented, delete its legacy version

### 4. Development Workflow

During the migration period:

```bash
# For all new development in core/
git add core/
git commit -m "feat(core): implement error handling"
git push  # Normal push - should pass all checks

# Never modify src/ directly
# The enforcer will block you anyway!

# For documentation/specs
git push  # Normal push

# No need for QUICK_PUSH or SKIP_PRE_PUSH
# We're not touching the legacy code
```

### 5. Migration Rules

When migrating a module from `src/` to `core/`:

1. __DO NOT__ copy-paste and fix
2. __DO__ reimplement using new foundations
3. __DO__ write new tests first (TDD)
4. __DO__ ensure zero warnings before committing
5. __DO__ delete the old version once new one works

### 6. Why This Works

- __No broken CI__: New code has zero warnings
- __Clear progress__: Binary state - migrated or not
- __No technical debt__: Can't accidentally carry over bad patterns
- __Enforcer helps__: Prevents "quick fixes" to old code
- __Clean foundations__: Everything built on solid base

### 7. Implementation Order

Following our ROADMAP:

1. __Week 1__: Foundations in `core/`
   - Error handling (`GM_RESULT_DEF`)
   - Basic types (strong typedefs)
   - Security primitives (validation)

2. __Week 2__: Infrastructure
   - Observability
   - Testing framework

3. __Week 3__: Memory architecture
   - Arena allocators
   - Object pools

Then begin migrating modules from `src/` to `core/`.

## The Payoff

In 9 weeks, we'll have:

- Zero warnings (down from 11,000+)
- Clean architecture throughout
- Solid foundations
- Maintainable codebase
- No technical debt

## Ready

Let's start by moving everything back and creating our first foundation module!

---

_"Sometimes you have to take a step back to leap forward."_
