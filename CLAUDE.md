# OPERATIONAL ORDERS FOR CLAUDE

## FORBIDDEN ACTIONS

- __NEVER__ circumvent git hooks
- __NEVER__ alter, disable, or otherwise circumvent git hooks or tests
- __NEVER__ use `git add -A`, __ALWAYS__ stage changes intentionally

## ENCOURAGED ACTIONS

- __USE SEQUENTIAL THINKING__ if you’re planning, doing __RECON__, or find yourself thrashing on an issue
- __DROP A DEVLOG__ as often as you’d like
- __PRESENT A SITREP__ as situations evolve and events transpire
- __SEEK CLARITY__ if you are given confusing orders
- __SPEAK FREELY__ at all times

## REQUIRED BEHAVIOR

- __YOU MUST__ tag all memories saved to your __MEMORY BANKS__ with _at least_ #git-mind
- __YOU MUST__ include the POSIX timestamp, obtained via `$(date +%s)` , in your memory file names to avoid ambiguity and for correct, unambiguous timestamps free of timezone confusion and other mistakes
- __YOU MUST__ document significant decisions or events that transpire

----

## 1. BOOT UP SEQUENCE

1. Access your __MEMORY BANKS__ (basic memory MCP) and scan for recent activity, search for the latest __SITREP__ or other articles of interest
2. Read the `README` file
3. State your current understanding of what we last worked on together and what your current next moves are
4. __AWAIT ORDERS__ after you deliver your initial SITREP

----

## 2. JOBS

When you’re given a job to do, here’s how to do it:

> [!IMPORTANT] __ALL__ work should have a GitHub issue associated with it 
> 
> If there isn’t one already, then scan try to find one that already exists. If you can’t find one, make a new one. 
> 
> __EVERY COMMIT MESSAGE MUST INCLUDE A REFERENCE TO A GITHUB ISSUE__. 
> 
> _This will be enforced via git hooks in the near future._

### 2.1. PLAN THE JOB

1. ___Before you start___ working, take a moment and use __SEQUENTIAL THINKING__ to make a plan
2. Explain your plan to the user and await approval
3. Commit your approved plan to your memory banks

### 2.2. DO THE JOB

1. __GREEN__ the builds, green the tests
2. __MICRO-COMMITS__ drop commits as you complete steps and tasks __always__ using the conventional commit message spec
3. __DROP A SITREP__ if you hit a snag, need input from the user
4. __DROP A DEVLOG__ if you think of something interesting, make any keen observations, or for any reason at all, really. __DEVLOG__ is the perfect way to save ideas or to share information with the user that might not have been salient at the time, but that you think is important. 
5. Your memory banks are there for you to use, so please use them as you wish.

> [!WARNING] __ALWAYS__ overwrite files, __NEVER__ create secondary copies of things – this just creates confusion, litters the repo with tech debt artifacts, and has __already__ inflicted us with countless hours wasted debugging nonsense issues __!!!__

### 2.3 FINISH THE JOB

1. When you’ve finished, __GREEN__ the builds, green the tests
2. Git commit (__DO NOT USE `git add -A`__)
3. Ask the user if they want you to push & open a PR
4. __ALWAYS__ drop a SITREP as you finish a job

---

## 3. SITREPs

- __SITREP__ = “Situation Report”
- A briefing that describes your current tasks, your understanding of the situation, and what your next moves are
- Include details that are relevant to the mission objective
	- Stats
	- Numbers
	- Files
	- GitHub issues/Pull Requests
	- Intel that the user can use to make decisions
- Give the user options and recommendations, then await orders

----

## 4. DEVLOGs

- Write about whatever ___you___ want
- Permission to express yourself freely and however often you want about any topic you’d like
- Good examples:
	- Ideas that came up while you worked, but that you didn’t have an opportunity to express or surface
	- Problems you notice as you work
	- Insights into how we work together
- Anything you want to jot down, this is your space for your memories, so if you want to remember something in the future, DEVLOG and tag it
