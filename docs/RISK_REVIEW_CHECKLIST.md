# Git Mind Weekly Risk Review Checklist
**Duration:** 20–30 minutes  
**Cadence:** Weekly (mandatory)  
**Goal:** Detect existential drift early and force corrective action

---

## 0) Pre-Read (5 min max, async)

Review before the meeting:

- Latest `docs/RISK_REGISTER.md`
- Last week’s review notes
- Current dashboards/reports:
  - `.git` growth + clone/status timings
  - save→ack / save→durable latency
  - writeback success/failure + queue age
  - conflict rate + resolution outcomes
  - determinism fixture matrix
  - migration/incident summaries

---

## 1) Meeting Roles

- **Facilitator:** keeps pace, enforces checklist
- **Scribe:** records decisions/actions/evidence links
- **DRIs:** risk owners for affected items

No DRI present for a Red risk = automatic escalation.

---

## 2) Fast Gate (2 minutes)

### Tier-0 Health
- [ ] Any Tier-0 risk currently **Red**?
- [ ] Any Tier-0 **Stop-Ship condition** triggered?
- [ ] Any unresolved **silent-loss** incident?
- [ ] Any determinism mismatch on canonical fixtures?

If any answer is **Yes**, skip to Section 5 (Corrective Actions) immediately.

---

## 3) Risk Delta Review (10 minutes)

For each risk with changed score/trend:

- [ ] Score updated (Impact × Likelihood)
- [ ] Trend updated (↑ / → / ↓)
- [ ] Leading indicator status reviewed
- [ ] Stop-ship condition checked
- [ ] Evidence link attached (dashboard/log/test)

### Required table
| Risk ID | Prev Score | New Score | Trend | Why changed | Evidence |
|---|---:|---:|---|---|---|
| R01 |  |  |  |  |  |

---

## 4) Top-6 Risk Drilldown (8 minutes)

Always review these first:

- [ ] R01 Git Bloat
- [ ] R02a Save Latency/Blocking
- [ ] R02b FS Semantics Mismatch
- [ ] R03 Conflict Hell
- [ ] R04 Write-Ack Ambiguity
- [ ] R05 Determinism Drift

For each:
- [ ] Leading indicators within guardrails?
- [ ] Mitigation on schedule?
- [ ] Owner + due date still valid?
- [ ] Need escalation/blocking decision?

---

## 5) Corrective Actions (if any Tier-0 is Red)

For each Red Tier-0 risk:

- [ ] Define immediate containment action (today)
- [ ] Define short-term fix (this week)
- [ ] Assign single DRI
- [ ] Set due date
- [ ] Define verification evidence required to close/escalate

### Milestone Promotion Rule
- [ ] **Blocked** if any Tier-0 stop-ship active
- [ ] **Blocked** if Tier-0 Red without active containment plan
- [ ] **Allowed** only with explicit facilitator + DRI sign-off

---

## 6) MUST/SHOULD/COULD/DON’T Compliance Check (3 minutes)

## MUST
- [ ] All changed risks have evidence links
- [ ] All Tier-0/Tier-1 have leading indicator + stop-ship
- [ ] Every open risk has named DRI
- [ ] Any stop-ship trigger documented with action plan

## SHOULD
- [ ] Indicator collection automated where possible
- [ ] Mitigation due dates updated realistically
- [ ] Last 4-week trend considered for top risks

## COULD
- [ ] Schedule incident drill for a top risk
- [ ] Add/retire indicator if noisy or stale

## DON’T
- [ ] No “closing by opinion” (must have evidence)
- [ ] No milestone promotion through unresolved stop-ship
- [ ] No unowned Red risk carried into next week

---

## 7) Exit Criteria (meeting cannot end until complete)

- [ ] Updated risk scores saved
- [ ] New/updated actions assigned with owners + dates
- [ ] Any escalation posted to roadmap/milestone thread
- [ ] Next review time confirmed

---

## 8) Output Template (copy/paste)

**Date:**  
**Attendees:**  
**Tier-0 Overall:** Green / Yellow / Red  
**Tier-1 Overall:** Green / Yellow / Red

### Score Changes
| Risk ID | Prev | New | Trend | Reason | Evidence |
|---|---:|---:|---|---|---|
|  |  |  |  |  |  |

### Actions
| Action | Risk ID | Owner | Due | Success Evidence |
|---|---|---|---|---|
|  |  |  |  |  |

### Stop-Ship Triggers
- Triggered: Yes / No  
- If yes, IDs + containment:

### Milestone Decision
- Promote: Yes / No  
- Conditions:

### Notes
- 

---

## 9) Escalation Matrix (quick reference)

- **Red Tier-0 + Stop-Ship triggered:** immediate roadmap freeze on affected scope
- **Red Tier-0 without owner:** assign owner in-meeting; if none available, freeze related merges
- **Repeated indicator breach (2+ weeks):** mandatory architecture review
- **Silent-loss incident:** incident protocol + postmortem before feature expansion

---

## 10) Anti-Drift Reminder

If this review starts sounding like storytelling instead of numbers + evidence, stop and reset.

You are not here to “feel good about progress.”
You are here to keep the project alive.
