# 🧭 CORE MIGRATION STATUS TRACKER

**Date Updated**: 2025-07-10  
**Phase**: Migration to Core Library (Phase 2)

## ✅ COMPLETED MIGRATIONS

### 1. **Edge Module** → `core/src/edge/` 
- **Status**: ✅ **COMPLETE** (PR #155 merged)
- **Files Migrated**:
  - `src/edge/edge.c` → `core/src/edge/edge.c`
  - `src/edge/attributed.c` → `core/src/edge/attributed.c`
- **Headers**: `core/include/gitmind/edge.h`, `core/include/gitmind/edge_attributed.h`
- **Tests**: `core/tests/unit/test_edge.c`
- **CI Status**: ✅ All green, zero warnings achieved

### 2. **CBOR Module** → `core/src/cbor/`
- **Status**: ✅ **COMPLETE** (Previously migrated)
- **Files Migrated**:
  - `src/cbor/*` → `core/src/cbor/cbor.c`
- **Headers**: `core/include/gitmind/cbor/`
- **Tests**: `core/tests/unit/test_cbor.c`
- **Note**: `src/cbor/` directory now empty

### 3. **Core Foundation Modules** → `core/src/`
- **Status**: ✅ **COMPLETE** (Previously migrated)
- **Modules**:
  - **Types**: `core/src/types/` (id, path, string, ulid, etc.)
  - **Crypto**: `core/src/crypto/` (backend, random, sha256)
  - **Error**: `core/src/error/error.c`
  - **I/O**: `core/src/io/io.c`
  - **Time**: `core/src/time/time.c`
  - **UTF-8**: `core/src/utf8/validate.c`

## 🎯 PENDING MIGRATIONS

### 4. **Attribution Module** → `core/src/attribution/`
- **Status**: ✅ **COMPLETE** (Previously migrated)
- **Files Migrated**:
  - `src/attribution/attribution.c` → `core/src/attribution/attribution.c`
- **Headers**: ✅ `core/include/gitmind/attribution.h`
- **Dependencies**: Used by edge module (already migrated)
- **Note**: Only documentation remains in `src/attribution/attribution.md`

### 5. **Cache System** → `core/src/cache/`
- **Status**: 🔄 **PENDING**
- **Files to Migrate**:
  - `src/cache/bitmap.c` → `core/src/cache/bitmap.c`
  - `src/cache/builder.c` → `core/src/cache/builder.c`
  - `src/cache/query.c` → `core/src/cache/query.c`
  - `src/cache/tree_builder.c` → `core/src/cache/tree_builder.c`
  - `src/cache/tree_size.c` → `core/src/cache/tree_size.c`
- **Headers**: Need creation in `core/include/gitmind/cache/`
- **Estimated Effort**: High (multiple files, complex system)

### 6. **Journal System** → `core/src/journal/`
- **Status**: 🔄 **PENDING**
- **Files to Migrate**:
  - `src/journal/reader.c` → `core/src/journal/reader.c`
  - `src/journal/writer.c` → `core/src/journal/writer.c`
- **Headers**: Need creation in `core/include/gitmind/journal/`
- **Estimated Effort**: Medium (2 files, well-defined interface)

### 7. **Hooks System** → `core/src/hooks/`
- **Status**: 🔄 **PENDING**
- **Files to Migrate**:
  - `src/hooks/augment.c` → `core/src/hooks/augment.c`
  - `src/hooks/post-commit.c` → `core/src/hooks/post-commit.c`
- **Headers**: `src/hooks/augment.h` → `core/include/gitmind/hooks/`
- **Estimated Effort**: Medium (2 files + header)

### 8. **Utility Functions** → `core/src/util/`
- **Status**: 🔄 **PENDING**
- **Files to Migrate**:
  - `src/util/error.c` → **CONFLICT**: Already have `core/src/error/error.c`
  - `src/util/random_default.c` → **ANALYZE**: May conflict with `core/src/crypto/random.c`
  - `src/util/sha.c` → **ANALYZE**: May conflict with `core/src/crypto/sha256.c`
- **Headers**: `src/util/gm_mem.h` → Needs analysis
- **Estimated Effort**: High (conflict resolution needed)

## 🚨 KNOWN CONFLICTS TO RESOLVE

1. **Error Handling**: `src/util/error.c` vs `core/src/error/error.c`
2. **Random Functions**: `src/util/random_default.c` vs `core/src/crypto/random.c`
3. **SHA Functions**: `src/util/sha.c` vs `core/src/crypto/sha256.c`

## 📋 MIGRATION CHECKLIST TEMPLATE

For each migration:
- [ ] Create migration branch: `migrate/{module}-to-core`
- [ ] Move source files: `src/{module}/*` → `core/src/{module}/`
- [ ] Create/update headers: `core/include/gitmind/{module}/`
- [ ] Update include paths in source files
- [ ] Create/migrate tests: `core/tests/unit/test_{module}.c`
- [ ] Update build system: `core/meson.build`
- [ ] Verify GNU CRY GAUNTLET (zero warnings)
- [ ] Pass all CI checks
- [ ] Create PR and merge
- [ ] Update this tracker

## 🎯 NEXT ACTION

**Proceed with Attribution module migration** - it's the logical next step with minimal conflicts.