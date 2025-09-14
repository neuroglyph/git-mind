#!/usr/bin/env python3
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
dirs = [ROOT / 'include', ROOT / 'core' / 'include']

bad = []
pat_ifndef = re.compile(r'^\s*#ifndef\s+([A-Za-z0-9_]+)')
req = re.compile(r'^GITMIND_')

for base in dirs:
    if not base.exists():
        continue
    for h in base.rglob('*.h'):
        try:
            text = h.read_text(encoding='utf-8', errors='ignore').splitlines()
        except Exception:
            continue
        guard = None
        for line in text[:50]:
            m = pat_ifndef.match(line)
            if m:
                guard = m.group(1)
                break
        if not guard or not req.match(guard):
            rel = h.relative_to(ROOT)
            bad.append((str(rel), guard or '(missing)'))

if bad:
    print('Header guard check failed for the following headers:')
    for path, guard in bad:
        print(f' - {path}: guard={guard} (expected prefix GITMIND_)')
    sys.exit(1)
else:
    print('Header guard check: OK')
    sys.exit(0)

