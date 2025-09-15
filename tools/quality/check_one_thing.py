#!/usr/bin/env python3
"""
Heuristic pre-commit check for the "One-Thing" policy.

Policy: If you touch a file that clearly contains multiple top-level types, you
should refactor the specific thing you touched into its own file. This checker
warns when a staged C/C header under include/ or core/ contains multiple top-
level typedef struct/enum definitions.

Rules of engagement:
- Only scans staged files in git index.
- Only checks files under include/, core/include/, core/src/ with extensions .h/.c.
- If a file has >1 occurrences of "typedef struct" or "typedef enum", we flag it.
- Bypass: set environment GM_ONE_THING_ALLOW=1 to skip (CI will still warn).

This is heuristic and advisory; it's here to nudge, not to be perfect.
"""
import os
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]

def staged_files():
    out = subprocess.check_output(['git', 'diff', '--cached', '--name-only', '--diff-filter=ACM']).decode()
    return [Path(p) for p in out.splitlines() if p]

def is_code_file(p: Path) -> bool:
    s = str(p)
    if not (s.startswith('include/') or s.startswith('core/include/') or s.startswith('core/src/')):
        return False
    return p.suffix in ('.h', '.c')

def count_defs(text: str) -> int:
    # crude but effective: count top-level typedef struct/enum occurrences
    return text.count('typedef struct') + text.count('typedef enum')

def main() -> int:
    if os.environ.get('GM_ONE_THING_ALLOW'):
        return 0
    failures = []
    for p in staged_files():
        if not is_code_file(p):
            continue
        try:
            text = p.read_text(encoding='utf-8', errors='ignore')
        except Exception:
            continue
        defs = count_defs(text)
        if defs > 1:
            failures.append(str(p))
    if failures:
        print('One-Thing policy advisory: files contain multiple top-level type definitions:')
        for f in failures:
            print(f'-', f)
        print('\nIf you touched a type in these files, please extract that type to its own header/source.\n'
              'To bypass for this commit, export GM_ONE_THING_ALLOW=1 (CI may still warn).')
        return 1
    return 0

if __name__ == '__main__':
    sys.exit(main())

