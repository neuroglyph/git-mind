# HN Launch: Required Features

## Must-Have (Launch Blockers)

### 1. __Time-Travel Core__

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

__Why critical__: This is the "holy shit" moment. Without this, it's just another file linker.

### 2. __Rock-Solid CLI Foundation__

- `git mind init` - Works in any Git repo
- `git mind link A B --type TYPE` - Creates semantic relationships
- `git mind list [--from FILE]` - Query the graph
- `git mind traverse FILE --depth N` - Explore connections
- `git mind check --fix` - Maintain integrity
- `git mind status` - Overview with stats

__Why critical__: HN users will immediately try to break it. It must be bulletproof.

### 3. __Performance That Impresses__

- <1ms startup time (measured and verified)
- <130KB binary size
- Works on repos with 10K+ files
- Handles 100+ links without slowdown

__Why critical__: HN loves technical efficiency. Slow = dismissed immediately.

### 4. __One-Line Install__

```bash
curl -fsSL https://get.gitmind.dev | bash
git mind version  # Confirms it works
```

__Why critical__: Friction kills HN engagement. Must be trivial to try.

### 5. __Visual Evolution Demo__

Something that shows understanding changing over time:

- ASCII timeline of when links were created/removed
- Confidence scores changing
- Maybe simple D3.js web view of graph evolution

__Why critical__: The time-travel concept needs to be viscerally obvious.

## Should-Have (Amplifies Impact)

### 6. __MCP Integration (Claude Memory)__

```bash
git mind claude-session --remember-context
# Claude builds on previous conversations
git mind claude-insights --show-patterns
```

__Why important__: This demonstrates AI that actually remembers. Huge differentiator.

### 7. __Cross-Repo Discovery__

```bash
git mind link ../other-repo/design.md src/implementation.rs --type implements
git mind network --show-cross-repo
```

__Why important__: Shows this scales beyond single projects.

### 8. __Confidence Tracking__

```bash
git mind link A B --confidence 0.7
git mind confidence A B --update 0.9 --note "verified correct"
git mind evolution --show-confidence-changes
```

__Why important__: Demonstrates understanding evolution, not just connection creation.

## Could-Have (Nice But Not Essential)

### 9. __Team Collaboration Preview__

```bash
git mind merge-understanding --from origin/alice-branch
git mind conflicts --resolve
```

__Why nice__: Shows collaborative potential without being demo-critical.

### 10. __Pattern Discovery__

```bash
git mind discover --patterns --confidence-threshold 0.8
# "Detected circular dependency: A -> B -> C -> A"
```

__Why nice__: Shows emergent intelligence, but complex to implement reliably.

### 11. __Rich Link Types__

Pre-defined types that make sense:

- `implements`, `references`, `depends_on`, `inspired_by`, `superseded_by`, `tests`, `documents`

__Why nice__: Makes demos more realistic and relatable.

## Demo Script Requirements

### The 2-Minute Video Must Show

1. __Install in 10 seconds__ (`curl | bash`)
2. __Link files in real repo__ (use git-mind's own repo)
3. __Time-travel__ (`git checkout HEAD~10 && git mind list`)
4. __Evolution visualization__ (how understanding changed)
5. __Performance__ (everything instant, no loading)
6. __If MCP works__: Claude remembering previous conversation

### Live Demo Must Survive

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

__If people can't see their understanding evolving over time in a real Git repo within 2 minutes of install, the launch fails.__

Everything else is secondary to making that core experience bulletproof and compelling.
