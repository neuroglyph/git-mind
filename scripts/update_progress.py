#!/usr/bin/env python3
"""
Update feature progress bars in docs/features/Features_Ledger.md and README.md.

Simple averaging of Progress % across group tables. Weighting/KLoC are TODO.
"""
from __future__ import annotations
import re
from pathlib import Path
import math

ROOT = Path(__file__).resolve().parents[1]
LEDGER = ROOT / "docs" / "features" / "Features_Ledger.md"
README = ROOT / "README.md"

GROUP_RE = re.compile(r"<!-- group-progress:([a-z0-9\-]+):begin -->\n```text\n(.*?)\n```\n<!-- group-progress:\1:end -->", re.S | re.I)
OVERALL_RE = re.compile(r"<!-- progress:begin -->\n```text\n(.*?)\n```\n<!-- progress:end -->", re.S)
READ_ME_BLOCK_RE = re.compile(r"<!-- features-progress:begin -->\n```text\n(.*?)\n```\n<!-- features-progress:end -->", re.S)

def progress_bar(pct: float, width: int = 40) -> str:
    filled = int(round(pct * width))
    filled = min(filled, width)
    return "█" * filled + ("▓" if filled < width and (pct*width - filled) > 0 else "") + "░" * (width - filled - (1 if filled < width and (pct*width - filled) > 0 else 0)) + f" {int(round(pct*100))}%"

def compute_group_percent(md: str, anchor: int) -> float:
    tail = md[anchor:]
    # find the feature table header
    m = re.search(r"^\|\s*Emoji\s*\|", tail, re.M)
    if not m:
        return 0.0
    i = m.start()
    table = tail[i:].splitlines()
    if len(table) < 3:
        return 0.0
    header = table[0]
    sep = table[1] if len(table) > 1 else ''
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
    vals = []
    weights = []
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
        # weight from KLoC
        w = 1.0
        if 0 <= kloc_idx < len(cells):
            try:
                kloc = float(cells[kloc_idx])
                w = max(0.5, 1.0 + math.log10(kloc * 1000 + 10.0) / 3.0)
            except Exception:
                w = 1.0
        vals.append(prog)
        weights.append(w)
    if not vals:
        return 0.0
    # weighted average
    wsum = sum(weights) if weights else float(len(vals))
    return (sum(p * w for p, w in zip(vals, weights)) / wsum) / 100.0

def update_ledger():
    text = LEDGER.read_text(encoding='utf-8')
    groups = []
    new = text
    for m in GROUP_RE.finditer(text):
        start = m.start()
        name = m.group(1)
        pct = compute_group_percent(text, m.end())
        bar = progress_bar(pct)
        block = f"<!-- group-progress:{name}:begin -->\n```text\n{bar}\n------------|-------------|------------|\n           MVP          Alpha    v1.0.0 \nfeatures=?\n```\n<!-- group-progress:{name}:end -->"
        new = new.replace(m.group(0), block)
        groups.append(pct)
    if groups:
        overall = sum(groups) / len(groups)
        over_m = OVERALL_RE.search(new)
        over_block = f"<!-- progress:begin -->\n```text\n{progress_bar(overall)}\n------------|-------------|------------|\n           MVP          Alpha    v1.0.0 \n```\n<!-- progress:end -->"
        if over_m:
            new = new.replace(over_m.group(0), over_block)
    if new != text:
        LEDGER.write_text(new, encoding='utf-8')

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
    README.write_text(text, encoding='utf-8')

def main():
    # Compute overall from ledger
    text = LEDGER.read_text(encoding='utf-8')
    groups = []
    weights = []
    for m in GROUP_RE.finditer(text):
        pct = compute_group_percent(text, m.end())
        groups.append(pct)
        weights.append(1.0)  # equal group weights for now; could sum KLoC per group
    overall = (sum(g * w for g, w in zip(groups, weights)) / sum(weights)) if groups else None
    update_ledger()
    update_readme(overall)

if __name__ == '__main__':
    main()
