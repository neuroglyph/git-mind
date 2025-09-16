#!/usr/bin/env python3
"""
Update feature progress bars in docs/features/Features_Ledger.md and README.md.

Simple averaging of Progress % across group tables. Weighting/KLoC are TODO.
"""
from __future__ import annotations
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
LEDGER = ROOT / "docs" / "features" / "Features_Ledger.md"
README = ROOT / "README.md"

GROUP_RE = re.compile(r"<!-- group-progress:([a-z0-9\-]+):begin -->\n```text\n(.*?)\n```\n<!-- group-progress:\1:end -->", re.S | re.I)

TABLE_RE = re.compile(r"^\|.*\n(?:\|[\-\s\|]+\|\n)(?P<body>(?:\|.*\n)+)", re.M)
ROW_RE = re.compile(r"\|.*?\|.*?\|.*?\|.*?\|.*?\|.*?\|.*?\|\s*(\d+)\s*(?:\%|\(|$)")

OVERALL_RE = re.compile(r"<!-- progress:begin -->\n```text\n(.*?)\n```\n<!-- progress:end -->", re.S)

def progress_bar(pct: float, width: int = 40) -> str:
    filled = int(round(pct * width))
    filled = min(filled, width)
    return "█" * filled + ("▓" if filled < width and (pct*width - filled) > 0 else "") + "░" * (width - filled - (1 if filled < width and (pct*width - filled) > 0 else 0)) + f" {int(round(pct*100))}%"

def compute_group_percent(md: str, anchor: int) -> float:
    # Find next table after anchor
    m = TABLE_RE.search(md, anchor)
    if not m:
        return 0.0
    body = m.group('body')
    vals = []
    for line in body.splitlines():
        rm = ROW_RE.search(line)
        if rm:
            vals.append(int(rm.group(1)))
    return (sum(vals) / len(vals) / 100.0) if vals else 0.0

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

def update_readme():
    if not README.exists():
        return
    text = README.read_text(encoding='utf-8')
    if not OVERALL_RE.search(text):
        # insert under Status section if present
        if "## 📊 Status" in text:
            parts = text.split("## 📊 Status", 1)
            progress = "\n<!-- features-progress:begin -->\n```text\nTo be updated by scripts/update_progress.py\n```\n<!-- features-progress:end -->\n"
            text = parts[0] + "## 📊 Status\n\n" + progress + parts[1]
            README.write_text(text, encoding='utf-8')

def main():
    update_ledger()
    update_readme()

if __name__ == '__main__':
    main()
