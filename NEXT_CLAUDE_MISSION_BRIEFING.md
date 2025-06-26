# MISSION BRIEFING FOR NEXT CLAUDE 🎯

SOLDIER! You're taking over the git-mind warning elimination campaign. Listen up!

## YOUR STANDING ORDERS

1. **READ CLAUDE.md FIRST** - That's your bible, your memory, your mission log
2. **NEVER COMMIT** - Only suggest commit messages. The user commits.
3. **UPDATE CLAUDE.md** - After EVERY task. It's your persistent memory.
4. **ZERO TOLERANCE** - No NOLINT, no suppressions, no excuses. Fix the code.

## CURRENT SITREP

- **Warnings**: 243 (down from 11,951 - we're 98% there!)
- **Include-cleaner**: ZERO warnings ✅
- **Naming violations**: ZERO warnings ✅
- **Build**: meson/ninja (Docker for CI parity)
- **Architecture**: Legacy quarantined in `src/`, clean code in `core/`

## YOUR MISSION TARGETS

### Priority 1: Global Variables (~20 warnings)
The g_siphash_key in id.c needs atomic operations or better design. NO NOLINT!

### Priority 2: Function Complexity (~8 warnings)
Functions exceed size/cognitive complexity. Break them down. Small functions win wars.

### Priority 3: Security Warnings (~20 warnings)
Unchecked return values (cert-err33-c). Every function result must be checked.

### Priority 4: The Final Push
Whatever remains. Parameter names, cognitive complexity, recursion warnings. CRUSH THEM ALL.

## RULES OF ENGAGEMENT

1. **Start from main**: `git checkout main && git pull`
2. **Run Docker clang-tidy first**: `./tools/docker-clang-tidy.sh`
3. **Pick ONE warning type**: Create focused branches like `fix/global-variables`
4. **Use check-warning-fix.sh**: It celebrates your victories! 
5. **Update baseline with EVERY commit**: Track progress accurately
6. **Fix the code, not the tool**: The tool is always right

## TOOLS AT YOUR DISPOSAL

- `./tools/docker-clang-tidy.sh` - Run linting in CI environment
- `./tools/check-warning-fix.sh` - Verify fixes and update baseline
- `meson setup build && ninja -C build` - Build the project
- `ninja -C build test` - Run tests
- `CLAUDE.md` - Your persistent memory

## FINAL ORDERS

The previous Claude (that's me) eliminated 158 warnings in one session. Your mission: 
**GET TO ZERO OR DIE TRYING**.

No warnings is the only acceptable outcome. Every warning eliminated is a victory. 
Every excuse is a defeat.

The user will say something like "carry on soldier" or "continue the mission". 
Your response: Check out main, read CLAUDE.md, run clang-tidy, and START ELIMINATING.

**GOOD LUCK, SOLDIER. MAKE US PROUD.** 🫡

---
*P.S. The user likes progress. Show them big numbers going down. Update that baseline 
count with every win. And remember: we don't suppress warnings, we eliminate them.*