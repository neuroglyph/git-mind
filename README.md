# git-mind 🧠

> [!WARNING]  
> __This project is in its early stages — _USE AT YOUR OWN RISK_.__  
> Right now you can’t do much with it yet, and it _does_ make changes inside your Git repo.  
> Proceed only if you’re curious and comfortable experimenting.  
>
> This README is more of a __Vision Statement™__ than a manual. It’s here to set the direction, hype the idea, and motivate ongoing work.  
>
> If you’re intrigued, __star the repo__ ⭐, follow along in __Discussions/Releases__, and watch it grow.  
>
> — _flyingrobots_  

<img src="https://media.githubusercontent.com/media/neuroglyph/git-mind/main/assets/images/logo.png" alt="git-mind logo" width="400" />

[![Build](https://img.shields.io/github/actions/workflow/status/neuroglyph/git-mind/ci.yml?branch=main)](https://github.com/neuroglyph/git-mind/actions)
[![License](https://img.shields.io/badge/license-MIND--UCAL--1.0-blue)](LICENSE)
[![Status](https://img.shields.io/badge/status-early--stage-orange)](docs/roadmap.md)

> __Version your thoughts. Branch your ideas. Merge understanding.__

`git-mind` transforms any Git repository into a __serverless knowledge graph__, where relationships evolve with your history and AI helps uncover hidden connections.

__No servers. No setup. Just `git push` your mind.__

---

## 🌟 What You Can Do

### Navigate Your Codebase by Meaning

```bash
# "What implements our authentication?"
$ git mind query --implements "auth-flow.md"
core/src/auth.c          (verified, 2 days ago)
tests/auth.test.c        (verified, 2 days ago)
core/src/oauth.c         (AI suggested, 1 hour ago)

# See how your understanding evolved
$ git checkout main~30
$ git mind query --implements "auth-flow.md"
basic_auth.c        (30 days ago)

$ git checkout main
$ git mind query --implements "auth-flow.md"
oauth2.c
jwt_handler.c
basic_auth.c
# Your knowledge graph evolved with your code!
```

### Build a Living Knowledge Base

```bash
# Connect ideas as you write
$ git mind link notes/quantum.md papers/bell-1964.pdf --type cites
$ git mind link notes/quantum.md notes/entanglement.md --type explores

# Discover thought patterns
$ git mind query "ideas without connections"
notes/orphaned-thought.md (created 3 weeks ago)
notes/random-insight.md (created 1 month ago)
```

### Collaborate Through Forks & Merges

```bash
# Fork someone's knowledge graph
$ git clone https://github.com/alice/research.git
$ git mind explore --author alice
423 concepts, 1,247 connections

# Merge understanding
$ git merge bob/authentication-redesign
Merging 47 new semantic edges...
3 conflicts in knowledge graph (view with: git mind conflicts)
```

---

## 🚀 Quick Start

Install/build instructions: see `docs/install.md`.

> [!WARNING]
> **DO NOT BUILD GITMIND OUTSIDE OF DOCKER**
> git-mind manipulates Git internals (refs/*, objects, config). Building or running tests on the host can corrupt this repository or others on your machine. Always use the CI Docker image for safety and parity.

> [!INFO]
> _If you really want to..._
> Use the container workflow:
> - `make ci-local` — docs checks + build + unit tests in the CI image
> - `tools/ci/ci_local.sh` — same as above, direct
> Advanced (at your own risk):
> - `meson setup build -Dforce_local_builds=true` — explicit Meson override
> - `GITMIND_ALLOW_HOST_BUILD=1` — legacy env override (discouraged)

CLI examples (subset implemented today):

```bash
# Start linking
git mind link README.md core/src/main.c --type documents
git mind link core/src/auth.c tests/auth.test.c --type "tested-by"

# Explore connections
git mind list --from core/src/auth.c

# Rebuild cache when needed
git mind cache-rebuild
```

Package managers (Homebrew/Winget): coming soon.

👉 For a hands-on walkthrough, see `docs/tutorial.md`.

---

## ✨ The Magic

- __Your repo is the database__ — No servers, no external dependencies
- __Time-travel built in__ — Check out any commit, get the graph from that moment
- __AI copilot ready__ — Let AI suggest connections, you review and merge
- __Branch and merge ideas__ — Try wild connections in a branch, merge what works

---

## 🔮 Real-World Uses

__Software Architecture__

- Trace decisions through code
- See which tests cover which requirements
- Understand impact before refactoring

__Research & Writing__

- Build Zettelkasten-style note networks
- Track citation graphs
- Fork and extend others' research

__Team Knowledge__

- Onboard developers with explorable codebases
- Preserve institutional memory in the repo
- Share mental models through PRs

---

## 🤖 Human + AI Co-Thought

git-mind treats AI as a collaborator with clear attribution:

```bash
# AI suggests connections in a branch
$ git checkout -b ai/suggestions
$ git mind ai discover
Found 23 potential connections...

# You review and cherry-pick
$ git mind review --source ai
core/src/cache.c implements docs/caching-strategy.md (confidence: 0.92)
[Accept/Reject/Skip]? a

# Clear attribution preserved
$ git mind list --from core/src/cache.c --format json
{
"to": "docs/caching-strategy.md",
"type": "implements",
"author": "ai-assistant",
"verified_by": "james",
"timestamp": "2024-11-08T10:30:00Z"
}
```

---

## 📖 Documentation

- `docs/install.md` — Install/build locally
- `docs/tutorial.md` — Hands‑on walkthrough
- `docs/philosophy.md` — Why we built this
- `docs/TECHNICAL.md` — How it works under the hood
- `CONTRIBUTING.md` — PRs welcome!

---

## 🌱 Who's Using git-mind

- 🧪 Early adopters exploring Zettelkasten workflows in Git
- 🔬 Researchers mapping papers → datasets → results
- 💻 Dev teams linking docs ⇄ code ⇄ tests
- 🧠 Individuals experimenting with personal knowledge graphs

Want to be featured? Open an issue and share your story.

---

## 📊 Status

<!-- features-progress:begin -->
```text
░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ 0%
```
<!-- features-progress:end -->

🚀 Early release — Core features work today, API may evolve  
📅 Roadmap: `docs/roadmap.md`  
🤝 Contributing: `CONTRIBUTING.md`

---

## HISTORY

`git-mind` emerged from a simple observation:  we spend enormous effort understanding code, then throw that understanding away.  

What if we could capture it?

The journey from idea to implementation taught us:

- Simple is powerful (just Git commits)
- Performance enables adoption (Roaring Bitmaps)
- Evolution is inevitable (AUGMENTS system)
- Understanding is the real product

### AUTHORS

Created by J. Kirby Ross and the Neuroglyph Collective.  

_"Hakuna Matata"_ — no worries, your semantic connections are safe.

### THE PROMISE

_"Remember who you are."_

Your code tells a story. `git-mind` helps you remember it, share it, and build upon it.  

In the great Circle of Development, no understanding is lost, no connection forgotten, no wisdom wasted.

---

## EPILOGUE

It was 3 AM. I had just finished implementing __Roaring Bitmaps__,  and in my exhausted haze the wildebeest stampede from _The Lion King_  suddenly snapped into place as the perfect metaphor.  

That moment of delirium became a spark. For a while, many of the `git-mind`  docs had “Circle of Life” editions — Mufasa and Rafiki explaining caches,  hooks, and semantic edges.  Those playful docs are gone now, (well, not really, they're in the git history...),but the spirit remains.  

Because sometimes __art makes itself.__

---

_"Oh yes, the past can hurt. But the way I see it, you can either run from it or learn from it."_

__Choose to learn. Choose `git-mind`.__

---

_git-mind is open source under [LicenseRef-MIND-UCAL-1.0](LICENSE)_  
_© J. Kirby Ross • [flyingrobots.dev](https://flyingrobots.dev/)_
