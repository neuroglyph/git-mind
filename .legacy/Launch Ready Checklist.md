# HN Launch: Required Features

## Must-Have (Launch Blockers)

### 1. **Time-Travel Core**

```bash
git mind init
git mind link A B --type implements
git commit -m "understanding v1"
# ... more commits, more links ...
git checkout HEAD~10
git mind list  # Shows old understanding
git checkout main
git mind evolution --show-changes  # Visual diff of how understanding evolved
```

**Why critical**: This is the "holy shit" moment. Without this, it's just another file linker.

### 2. **Rock-Solid CLI Foundation**

- `git mind init` - Works in any Git repo
- `git mind link A B --type TYPE` - Creates semantic relationships
- `git mind list [--from FILE]` - Query the graph
- `git mind traverse FILE --depth N` - Explore connections
- `git mind check --fix` - Maintain integrity
- `git mind status` - Overview with stats

**Why critical**: HN users will immediately try to break it. It must be bulletproof.

### 3. **Performance That Impresses**

- <1ms startup time (measured and verified)
- <130KB binary size
- Works on repos with 10K+ files
- Handles 100+ links without slowdown

**Why critical**: HN loves technical efficiency. Slow = dismissed immediately.

### 4. **One-Line Install**

```bash
curl -fsSL https://get.gitmind.dev | bash
git mind version  # Confirms it works
```

**Why critical**: Friction kills HN engagement. Must be trivial to try.

### 5. **Visual Evolution Demo**

Something that shows understanding changing over time:

- ASCII timeline of when links were created/removed
- Confidence scores changing
- Maybe simple D3.js web view of graph evolution

**Why critical**: The time-travel concept needs to be viscerally obvious.

## Should-Have (Amplifies Impact)

### 6. **MCP Integration (Claude Memory)**

```bash
git mind claude-session --remember-context
# Claude builds on previous conversations
git mind claude-insights --show-patterns
```

**Why important**: This demonstrates AI that actually remembers. Huge differentiator.

### 7. **Cross-Repo Discovery**

```bash
git mind link ../other-repo/design.md src/implementation.rs --type implements
git mind network --show-cross-repo
```

**Why important**: Shows this scales beyond single projects.

### 8. **Confidence Tracking**

```bash
git mind link A B --confidence 0.7
git mind confidence A B --update 0.9 --note "verified correct"
git mind evolution --show-confidence-changes
```

**Why important**: Demonstrates understanding evolution, not just connection creation.

## Could-Have (Nice But Not Essential)

### 9. **Team Collaboration Preview**

```bash
git mind merge-understanding --from origin/alice-branch
git mind conflicts --resolve
```

**Why nice**: Shows collaborative potential without being demo-critical.

### 10. **Pattern Discovery**

```bash
git mind discover --patterns --confidence-threshold 0.8
# "Detected circular dependency: A -> B -> C -> A"
```

**Why nice**: Shows emergent intelligence, but complex to implement reliably.

### 11. **Rich Link Types**

Pre-defined types that make sense:

- `implements`, `references`, `depends_on`, `inspired_by`, `superseded_by`, `tests`, `documents`

**Why nice**: Makes demos more realistic and relatable.

## Demo Script Requirements

### The 2-Minute Video Must Show:

1. **Install in 10 seconds** (`curl | bash`)
2. **Link files in real repo** (use git-mind's own repo)
3. **Time-travel** (`git checkout HEAD~10 && git mind list`)
4. **Evolution visualization** (how understanding changed)
5. **Performance** (everything instant, no loading)
6. **If MCP works**: Claude remembering previous conversation

### Live Demo Must Survive:

- Works on Mac, Linux, Windows (WSL)
- Works on repos with thousands of files
- Handles edge cases (broken links, conflicts, etc.)
- Network issues don't break install
- Can be run multiple times safely

## Technical Readiness Checklist

- [ ] Binary builds reliably across platforms
- [ ] Install script handles all major systems
- [ ] All core commands work flawlessly
- [ ] Memory leaks eliminated (valgrind clean)
- [ ] Performance benchmarks documented
- [ ] Works with Git 2.0+ (backward compatibility)
- [ ] Handles Unicode filenames correctly
- [ ] No external dependencies beyond libc
- [ ] Error messages are helpful, not cryptic

## Marketing Readiness Checklist

- [ ] get.gitmind.dev domain serves install script
- [ ] GitHub releases have binaries for all platforms
- [ ] README has 30-second quick start
- [ ] Screencast shows time-travel in under 60 seconds
- [ ] HN title tested for maximum engagement
- [ ] Comments strategy prepared for common questions

## The Launch Killer

**If people can't see their understanding evolving over time in a real Git repo within 2 minutes of install, the launch fails.**

Everything else is secondary to making that core experience bulletproof and compelling.