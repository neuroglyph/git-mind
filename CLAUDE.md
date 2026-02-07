# OPERATIONAL ORDERS FOR CLAUDE

## CROSS-REFERENCE

- Code standards and contribution workflow: [CONTRIBUTING.md](CONTRIBUTING.md)
- User-facing documentation: [GUIDE.md](GUIDE.md)

## FORBIDDEN ACTIONS

- **NEVER** circumvent git hooks
- **NEVER** use `git add -A` — always stage changes intentionally
- **NEVER** commit files that contain secrets (.env, credentials, etc.)

## ENCOURAGED ACTIONS

- **USE SEQUENTIAL THINKING** if you're planning, doing recon, or find yourself thrashing
- **DROP A DEVLOG** as often as you'd like
- **PRESENT A SITREP** as situations evolve
- **SEEK CLARITY** if you are given confusing orders
- **SPEAK FREELY** at all times

## REQUIRED BEHAVIOR

- **YOU MUST** tag all memories saved to your memory banks with at least `#git-mind`
- **YOU MUST** include the POSIX timestamp (via `$(date +%s)`) in memory file names
- **YOU MUST** document significant decisions or events
- **YOU MUST** reference a GitHub issue in every commit message

---

## 1. BOOT UP SEQUENCE

1. Access your memory banks and scan for recent activity (latest SITREP or relevant notes)
2. Read the README
3. State your current understanding of what we last worked on and your next moves
4. **AWAIT ORDERS** after you deliver your initial SITREP

---

## 2. JOBS

> All work should have a GitHub issue associated with it. If there isn't one, find or create one. Every commit message must reference an issue.

### 2.1. PLAN THE JOB

1. Before starting, use sequential thinking to make a plan
2. Explain your plan to the user and await approval
3. Commit your approved plan to your memory banks

### 2.2. DO THE JOB

1. Green the builds, green the tests
2. Drop micro-commits as you complete steps — always use conventional commit format
3. Drop a SITREP if you hit a snag or need input
4. Drop a DEVLOG for ideas, observations, or anything you want to remember
5. Use your memory banks freely

> **ALWAYS** overwrite files, **NEVER** create secondary copies — that creates confusion and tech debt.

### 2.3. FINISH THE JOB

1. Green the builds, green the tests
2. Git commit (do NOT use `git add -A`)
3. Ask the user if they want you to push and open a PR
4. Drop a SITREP as you finish

---

## 3. SITREPs

A briefing covering:
- Current tasks and situation understanding
- Relevant stats, files, issues, PRs
- Intel the user can use to make decisions
- Options and recommendations, then await orders

## 4. DEVLOGs

Your space. Write about whatever you want:
- Ideas that came up while working
- Problems you notice
- Insights about collaboration
- Anything you want to remember later

---

## 5. TECH STACK REFERENCE

- **Runtime**: Node.js >= 20, ES modules
- **Core dependency**: `@git-stunts/git-warp` (local path, CRDT graph on Git)
- **Plumbing**: `@git-stunts/plumbing` (must be installed as direct dependency)
- **Tests**: vitest
- **Style**: Plain JS with JSDoc, no TypeScript
- **CLI**: Manual argv parsing, no CLI frameworks
- **Formatting**: chalk + figures for terminal output
