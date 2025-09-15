#!/usr/bin/env python3
"""
Check that selected documentation Markdown files contain a YAML frontmatter block
with required keys. Designed to be simple and fast, matching our repo layout.

Usage:
  python3 tools/docs/check_frontmatter.py [--base docs]

Exit codes:
  0 = all good
  1 = problems found
"""

from __future__ import annotations
import argparse
import pathlib
import sys
from typing import List, Tuple


REQUIRED_KEYS = [
    "title",
    "description",
    "audience",
    "domain",
    "tags",
    "status",
]

# Directories to include for enforcement
INCLUDE_DIRS = {
    "docs/architecture": True,
    "docs/cli": True,
    "docs/operations": True,
    "docs/quality": True,
    "docs/testing": True,
    "docs/PRDs": True,
    "docs/adr": True,
    "docs/requirements": True,
    "docs/deployment": True,
    "docs/charter": True,
    "docs/api": True,
    "docs/risk": True,
    "docs/specs": True,
}

# Specific files to always include (live at docs root)
ALWAYS_INCLUDE = {
    "docs/README.md",
    "docs/install.md",
    "docs/tutorial.md",
    "docs/philosophy.md",
    "docs/TECHNICAL.md",
    "docs/roadmap.md",
    "docs/operations/Environment_Variables.md",
}

# Subtrees to exclude (journals, brainstorming, reviews, etc.)
EXCLUDE_DIR_PREFIXES = (
    "docs/enforcer/",
    "docs/sitrep/",
    "docs/wish-list-features/",
    "docs/talk-shop/",
    "docs/audits/",
    "docs/code-review/",
    "docs/code-reviews/",
    "docs/hn-demo/",
    "docs/planning/",
)


def should_check(path: pathlib.Path) -> bool:
    p = str(path).replace("\\", "/")
    if not p.endswith(".md"):
        return False
    for pref in EXCLUDE_DIR_PREFIXES:
        if p.startswith(pref):
            return False
    # Enforce only for included dirs
    # Always include select root docs
    if p in ALWAYS_INCLUDE:
        return True
    for inc in INCLUDE_DIRS.keys():
        if p.startswith(inc + "/") or p == inc + ".md":
            return True
    return False


def parse_frontmatter(text: str) -> Tuple[bool, List[str]]:
    """Return (ok, missing_keys). Accepts HTML comments or blank lines before frontmatter.
    Frontmatter format:
    ---\n
    key: value\n
    ...\n
    ---
    """
    lines = text.splitlines()
    i = 0
    # Skip leading blank and HTML comment lines
    while i < len(lines) and (not lines[i].strip() or lines[i].lstrip().startswith("<!--")):
        i += 1
    if i >= len(lines) or lines[i].strip() != "---":
        return False, REQUIRED_KEYS[:]  # no frontmatter
    i += 1
    keys_found = set()
    while i < len(lines) and lines[i].strip() != "---":
        line = lines[i]
        if ":" in line:
            key = line.split(":", 1)[0].strip()
            if key:
                keys_found.add(key)
        i += 1
    missing = [k for k in REQUIRED_KEYS if k not in keys_found]
    return len(missing) == 0, missing


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--base", default="docs", help="Base directory to scan")
    args = ap.parse_args()

    base = pathlib.Path(args.base)
    if not base.exists():
        print(f"Base path not found: {base}", file=sys.stderr)
        return 1

    failures: List[str] = []
    for path in base.rglob("*.md"):
        rel = path.relative_to(pathlib.Path.cwd()) if path.is_absolute() else path
        rel_str = str(rel).replace("\\", "/")
        if not should_check(pathlib.Path(rel_str)):
            continue
        text = path.read_text(encoding="utf-8", errors="ignore")
        ok, missing = parse_frontmatter(text)
        if not ok:
            failures.append(f"{rel_str} -> missing keys: {', '.join(missing)}")

    if failures:
        print("Frontmatter check failed for:")
        for f in failures:
            print(f"- {f}")
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
