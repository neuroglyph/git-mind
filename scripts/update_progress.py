#!/usr/bin/env python3
"""
Update feature progress bars in docs/features/Features_Ledger.md and README.md.

Progress is derived from per-feature percentages using KLoC as weights. Milestone
bars aggregate features by milestone, while the overall bar blends milestones via
configurable weights.
"""
from __future__ import annotations
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
LEDGER = ROOT / "docs" / "features" / "Features_Ledger.md"
README = ROOT / "README.md"

GROUP_RE = re.compile(r"<!-- group-progress:([a-z0-9\-]+):begin -->\n```text\n(.*?)\n```\n<!-- group-progress:\1:end -->", re.S | re.I)
OVERALL_RE = re.compile(r"<!-- progress:begin -->\n```text\n(.*?)\n```\n<!-- progress:end -->", re.S)
READ_ME_BLOCK_RE = re.compile(r"<!-- features-progress:begin -->\n```text\n(.*?)\n```\n<!-- features-progress:end -->", re.S)
OVERALL_SECTION_RE = re.compile(r"<!-- progress-overall:begin -->\n```text\n(.*?)\n```\n<!-- progress-overall:end -->", re.S)
MILESTONE_RE = {
    key: re.compile(rf"<!-- progress-{key.lower()}:begin -->\n```text\n(.*?)\n```\n<!-- progress-{key.lower()}:end -->", re.S)
    for key in ("mvp", "alpha", "beta", "v1")
}
TASK_BLOCK_RE = {
    key: re.compile(rf"> <!-- tasks-{key}:begin -->\n(.*?)\n> <!-- tasks-{key}:end -->", re.S)
    for key in ("mvp", "alpha", "beta", "v1")
}

MILESTONE_LABELS = {
    "MVP": "mvp",
    "Alpha": "alpha",
    "Beta": "beta",
    "v1.0.0": "v1",
    "V1": "v1",
}

MILESTONE_WEIGHTS = {
    "mvp": 0.3,
    "alpha": 0.3,
    "beta": 0.2,
    "v1": 0.2,
}

def progress_bar(pct: float, width: int = 40) -> str:
    pct = max(0.0, min(1.0, pct))
    filled = int(round(pct * width))
    filled = min(filled, width)
    remainder = pct * width - filled
    edge = 1 if filled < width and remainder > 0 else 0
    return "█" * filled + ("▓" if edge else "") + "░" * (width - filled - edge) + f" {int(round(pct * 100))}%"

def compute_group_percent(md: str, anchor: int, entries: list[tuple[str, float, float]] | None = None) -> tuple[float, float, int]:
    tail = md[anchor:]
    # find the feature table header
    m = re.search(r"^\|\s*Emoji\s*\|", tail, re.M)
    if not m:
        return 0.0, 0.0, 0
    i = m.start()
    table = tail[i:].splitlines()
    if len(table) < 3:
        return 0.0, 0.0, 0
    header = table[0]
    rows = table[2:]
    header_cells = [c.strip() for c in header.strip().split('|')]
    # locate column indices
    def find_idx(name_sub: str) -> int:
        for idx, c in enumerate(header_cells):
            if name_sub.lower() in c.lower():
                return idx
        return -1
    progress_idx = find_idx('Progress')
    kloc_idx = find_idx('KLoC')
    milestone_idx = find_idx('Milestone')
    vals: list[float] = []
    weights: list[float] = []
    count = 0
    for line in rows:
        if not line.startswith('|'):
            break
        cells = [c.strip() for c in line.strip().split('|')]
        prog = None
        # prefer indexed cell
        if 0 <= progress_idx < len(cells):
            c = cells[progress_idx]
            if c.endswith('%') and c[:-1].strip().isdigit():
                prog = int(c[:-1].strip())
            else:
                s2 = ''.join(ch for ch in c if ch.isdigit())
                if s2:
                    prog = int(s2)
        # fallback scan
        if prog is None:
            for c in cells:
                if c.endswith('%') and c[:-1].strip().isdigit():
                    prog = int(c[:-1].strip())
                    break
        if prog is None:
            continue
        # weight from KLoC (default small weight so features without data still count)
        w = 0.1
        if 0 <= kloc_idx < len(cells):
            try:
                raw = cells[kloc_idx].replace(',', '')
                match = re.search(r"-?\d+(?:\.\d+)?", raw)
                if match:
                    kloc = float(match.group(0))
                    if kloc > 0:
                        w = kloc
            except Exception:
                w = 0.1
        vals.append(prog)
        weights.append(w)
        count += 1
        if entries is not None:
            milestone = "Unassigned"
            if 0 <= milestone_idx < len(cells):
                milestone = cells[milestone_idx] or "Unassigned"
            entries.append((milestone.strip(), prog / 100.0, w))
    if not vals:
        return 0.0, 0.0, 0
    wsum = sum(weights)
    if wsum <= 0:
        pct = sum(vals) / len(vals) / 100.0
    else:
        pct = (sum(p * w for p, w in zip(vals, weights)) / wsum) / 100.0
    return pct, wsum, count

def extract_tasks(md: str) -> dict[str, list[str]]:
    tasks: dict[str, list[str]] = {k: [] for k in MILESTONE_RE.keys()}
    lower = md.lower()
    if "## tasklist" not in lower:
        return tasks
    start = lower.index("## tasklist")
    segment = md[start:]
    for line in segment.splitlines():
        stripped = line.strip()
        if stripped.startswith("- [ ]"):
            tag = None
            rest = stripped[len("- [ ]"):].strip()
            if rest.startswith("[") and "]" in rest:
                tag_name = rest[1:rest.index("]")]
                key = MILESTONE_LABELS.get(tag_name, None)
                if key:
                    tag = key
                    rest = rest[rest.index("]") + 1:].strip()
                    entry = f"- [ ] {rest}" if rest else "- [ ]"
                else:
                    entry = stripped
            else:
                entry = stripped
            if not tag:
                tag = "mvp"
            tasks.setdefault(tag, []).append(entry if tag in tasks else stripped)
    return tasks


def update_ledger() -> tuple[float | None, dict[str, float], dict[str, list[str]]]:
    text = LEDGER.read_text(encoding='utf-8')
    groups = []
    new = text
    feature_entries: list[tuple[str, float, float]] = []
    for m in GROUP_RE.finditer(text):
        start = m.start()
        name = m.group(1)
        pct, weight, count = compute_group_percent(text, m.end(), feature_entries)
        bar = progress_bar(pct)
        feature_tag = f"features={count}" if count else "features=0"
        block = (
            f"<!-- group-progress:{name}:begin -->\n" \
            "```text\n" \
            f"{bar}\n" \
            "------------|-------------|------------|\n" \
            "           MVP          Alpha    v1.0.0 \n" \
            f"{feature_tag}\n" \
            "```\n" \
            f"<!-- group-progress:{name}:end -->"
        )
        new = new.replace(m.group(0), block)
        groups.append((pct, weight if weight > 0 else float(count or 0)))
    milestone_totals: dict[str, tuple[float, float]] = {
        key: (0.0, 0.0) for key in MILESTONE_RE.keys()
    }
    for raw_label, pct, weight in feature_entries:
        key = MILESTONE_LABELS.get(raw_label or "", None)
        if not key:
            continue
        total_pct, total_weight = milestone_totals[key]
        milestone_totals[key] = (total_pct + pct * weight, total_weight + weight)
    milestone_progress: dict[str, float] = {}
    for key, (total, weight) in milestone_totals.items():
        if weight > 0:
            milestone_progress[key] = total / weight
        else:
            milestone_progress[key] = 0.0
    overall = None
    if milestone_progress:
        total_weight = sum(MILESTONE_WEIGHTS.values())
        if total_weight > 0:
            overall = sum(milestone_progress.get(k, 0.0) * w for k, w in MILESTONE_WEIGHTS.items()) / total_weight
    over_m = OVERALL_SECTION_RE.search(new)
    if over_m:
        legend = " | ".join(
            f"{name.upper()} {int(round(milestone_progress.get(key, 0.0) * 100))}%"
            for name, key in (("MVP", "mvp"), ("Alpha", "alpha"), ("Beta", "beta"), ("v1.0.0", "v1"))
        )
        over_block = (
            "<!-- progress-overall:begin -->\n"
            "```text\n"
            f"{progress_bar(overall or 0.0)}\n"
            f"{legend}\n"
            "```\n"
            "<!-- progress-overall:end -->"
        )
        new = new.replace(over_m.group(0), over_block)
    for label, regex in MILESTONE_RE.items():
        match = regex.search(new)
        if not match:
            continue
        pct = milestone_progress.get(label, 0.0)
        block = (
            f"<!-- progress-{label}:begin -->\n"
            "```text\n"
            f"{progress_bar(pct)}\n"
            "```\n"
            f"<!-- progress-{label}:end -->"
        )
        new = new.replace(match.group(0), block)
    tasks = extract_tasks(new)
    for label, regex in TASK_BLOCK_RE.items():
        match = regex.search(new)
        if not match:
            continue
        items = tasks.get(label, []) if tasks else []
        if not items:
            body = "> _All tracked tasks complete_"
        else:
            body = "\n".join(f"> {item}" for item in items)
        replacement = f"> <!-- tasks-{label}:begin -->\n{body}\n> <!-- tasks-{label}:end -->"
        new = new.replace(match.group(0), replacement)
    if new != text:
        LEDGER.write_text(new, encoding='utf-8')
    return overall, milestone_progress, tasks

def update_readme(overall: float | None = None):
    if not README.exists():
        return
    text = README.read_text(encoding='utf-8')
    if "## 📊 Status" not in text:
        return
    if not READ_ME_BLOCK_RE.search(text):
        parts = text.split("## 📊 Status", 1)
        progress = "\n<!-- features-progress:begin -->\n```text\nFeature progress to be updated via scripts/update_progress.py\n```\n<!-- features-progress:end -->\n\n"
        text = parts[0] + "## 📊 Status\n\n" + progress + parts[1]
    if overall is not None:
        block = f"<!-- features-progress:begin -->\n```text\n{progress_bar(overall)}\n```\n<!-- features-progress:end -->\n"
        # Remove all existing blocks, then insert one
        text = READ_ME_BLOCK_RE.sub("", text)
        # Insert just after the Status header
        parts = text.split("## 📊 Status", 1)
        if len(parts) == 2:
            text = parts[0] + "## 📊 Status\n\n" + block + "\n" + parts[1]
    text = re.sub(r"\n{3,}", "\n\n", text)
    README.write_text(text, encoding='utf-8')

def main():
    overall, _, _ = update_ledger()
    update_readme(overall)

if __name__ == '__main__':
    main()
