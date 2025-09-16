#!/usr/bin/env python3
"""
Curate AGENTS.md recent developments and archive older activity.

Behavior:
- Extract date-stamped entries from all "## Agent Activity Log" sections.
- Keep only the most recent date's entries in a "## Recent Developments" section
  near the top (as a machine-readable YAML block + bullets).
- Archive older date sections into docs/activity/YYYY-MM.md, grouped by month.
- Replace the original "## Agent Activity Log" content with a short pointer to archives.

Idempotent: safe to run multiple times; only rewrites when changes are needed.
"""
from __future__ import annotations

import re
import sys
from pathlib import Path
from datetime import datetime, timezone

ROOT = Path(__file__).resolve().parents[2]
AGENTS = ROOT / "AGENTS.md"
ARCHIVE_DIR = ROOT / "docs" / "activity"

HEADER_RECENT = "## Recent Developments"
HEADER_ACTIVITY = "## Agent Activity Log"
HEADER_TLDR = "## Agent TL;DR (read me first)"
HEADER_BOOT = "## Boot Checklist"

DATE_RE = re.compile(r"^(\d{4}-\d{2}-\d{2})\s*$")


def split_sections(lines: list[str], header: str) -> list[tuple[int, int]]:
    """Return list of (start, end) indexes for sections matching the header."""
    indices = []
    i = 0
    n = len(lines)
    while i < n:
        if lines[i].strip() == header:
            # section from i to next top-level header or end
            j = i + 1
            while j < n and not (lines[j].startswith("## ") and lines[j].strip() != header):
                j += 1
            indices.append((i, j))
            i = j
        else:
            i += 1
    return indices


def extract_date_blocks(section_lines: list[str]) -> list[tuple[str, list[str]]]:
    """Extract (date, block_lines) from a section body containing date headings."""
    blocks = []
    i = 0
    n = len(section_lines)
    while i < n:
        m = DATE_RE.match(section_lines[i].strip()) if section_lines[i].strip() else None
        if m:
            date = m.group(1)
            j = i + 1
            # until next date or EOF or next top-level header (shouldn't be here)
            while j < n:
                line = section_lines[j]
                if DATE_RE.match(line.strip()):
                    break
                if line.startswith("## "):
                    break
                j += 1
            blocks.append((date, section_lines[i:j]))
            i = j
        else:
            i += 1
    return blocks


def first_bullets(block_lines: list[str], limit: int = 6) -> list[str]:
    out = []
    for line in block_lines:
        s = line.strip()
        if s.startswith("-"):
            out.append(s[1:].strip())
            if len(out) >= limit:
                break
    return out


def _render_recent_yaml(date: str, bullets: list[str]) -> list[str]:
    import textwrap
    out = ["```yaml\n", "recent_developments:\n"]
    for b in bullets:
        out.append(f"  - date: {date}\n")
        out.append("    summary: >\n")
        for line in textwrap.wrap(b, width=100):
            out.append("      " + line + "\n")
    out.append("```\n")
    return out


def _section_range(lines: list[str], header: str):
    n = len(lines)
    i = 0
    start = end = None
    while i < n:
        if lines[i].strip() == header:
            start = i
            j = i + 1
            while j < n and not (lines[j].startswith("## ") and lines[j].strip() != header):
                j += 1
            end = j
            break
        i += 1
    return (start, end) if start is not None else (None, None)


def ensure_recent_developments(lines: list[str], date: str, bullets: list[str]) -> list[str]:
    rendered = [HEADER_RECENT + "\n", "\n"] + _render_recent_yaml(date, bullets) + ["\n"]

    start, end = _section_range(lines, HEADER_RECENT)
    if start is not None:
        return lines[:start] + rendered + lines[end:]

    # Prefer insertion after Boot Checklist, else after TLDR, else near top
    _, boot_end = _section_range(lines, HEADER_BOOT)
    if boot_end is not None:
        insert_at = boot_end
        return lines[:insert_at] + ["\n"] + rendered + lines[insert_at:]

    _, tldr_end = _section_range(lines, HEADER_TLDR)
    if tldr_end is not None:
        insert_at = tldr_end
        return lines[:insert_at] + ["\n"] + rendered + lines[insert_at:]

    insert_at = 1 if len(lines) > 1 else 0
    return lines[:insert_at] + ["\n"] + rendered + lines[insert_at:]


def archive_blocks(blocks: list[tuple[str, list[str]]]):
    ARCHIVE_DIR.mkdir(parents=True, exist_ok=True)
    for date, content in blocks:
        dt = datetime.strptime(date, "%Y-%m-%d")
        fname = ARCHIVE_DIR / f"{dt.year}-{dt.month:02d}.md"
        if not fname.exists():
            fname.write_text(
                "---\n"
                f"title: Activity {dt.year}-{dt.month:02d}\n"
                "description: Archived agent activity logs.\n"
                "audience: [contributors]\n"
                "domain: [quality]\n"
                "tags: [activity]\n"
                "status: archive\n"
                f"last_updated: {datetime.now(timezone.utc).date()}\n"
                "---\n\n",
                encoding="utf-8",
            )
        with fname.open("a", encoding="utf-8") as f:
            f.write(f"## {date}\n\n")
            f.writelines(content)
            if not content[-1].endswith("\n"):
                f.write("\n")
            f.write("\n")


def curate():
    if not AGENTS.exists():
        return 0
    orig = AGENTS.read_text(encoding="utf-8")
    lines = orig.splitlines(keepends=True)

    # Find all Agent Activity Log sections and collect date blocks
    sections = split_sections(lines, HEADER_ACTIVITY)
    if not sections:
        return 0

    all_blocks: list[tuple[str, list[str], tuple[int, int]]] = []
    for (s, e) in sections:
        body = lines[s + 1:e]
        blocks = extract_date_blocks(body)
        for date, content in blocks:
            all_blocks.append((date, content, (s, e)))

    if not all_blocks:
        return 0

    # Determine most recent date
    all_blocks.sort(key=lambda x: x[0])
    latest_date = all_blocks[-1][0]

    latest_blocks = [(d, c) for (d, c, _) in all_blocks if d == latest_date]
    older_blocks = [(d, c) for (d, c, _) in all_blocks if d != latest_date]

    # Build bullets from first latest block
    bullets = first_bullets(latest_blocks[0][1]) if latest_blocks else []
    new_lines = ensure_recent_developments(lines, latest_date, bullets)

    # Replace activity sections with pointer
    pointer = [HEADER_ACTIVITY + "\n", "\n", "See archives under `docs/activity/` for older logs.\n", "\n"]
    # Remove existing activity sections and insert pointer once (after first occurrence)
    secs = split_sections(new_lines, HEADER_ACTIVITY)
    if secs:
        s0, e0 = secs[0]
        new_lines = new_lines[:s0] + pointer + new_lines[e0:]
        # Remove any additional activity sections
        while True:
            more = split_sections(new_lines, HEADER_ACTIVITY)
            if len(more) <= 1:
                break
            s, e = more[1]
            new_lines = new_lines[:s] + new_lines[e:]

    # Archive older blocks
    if older_blocks:
        archive_blocks(older_blocks)

    new_text = "".join(new_lines)
    if new_text != orig:
        AGENTS.write_text(new_text, encoding="utf-8")
        # Signal to pre-commit that files changed; it will re-run
        print("Curated Recent Developments and archived older activity logs.")
        return 1
    return 0


def main():
    sys.exit(curate())


if __name__ == "__main__":
    main()
