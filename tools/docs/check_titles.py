#!/usr/bin/env python3
"""
Verify that each Markdown document's first H1 matches its front-matter title.

Exits non-zero and prints offending files.
"""
import os
import sys
import pathlib


def parse_front_matter(text: str):
    lines = text.splitlines()
    if len(lines) < 3 or lines[0].strip() != '---':
        return {}
    fm = {}
    for i in range(1, len(lines)):
        if lines[i].strip() == '---':
            break
        line = lines[i]
        if ':' in line:
            k, v = line.split(':', 1)
            fm[k.strip()] = v.strip().strip('"')
    return fm


def first_h1(text: str):
    for line in text.splitlines():
        s = line.strip()
        if s.startswith('# '):
            return s[2:].strip()
    return ''


def main():
    base = pathlib.Path('docs')
    bad = []
    for p in base.rglob('*.md'):
        # Skip templates and rejected suggestions folder
        pp = str(p)
        if '/templates/' in pp or '/rejected-suggestions/' in pp or '/code-reviews/' in pp:
            continue
        text = p.read_text(encoding='utf-8')
        fm = parse_front_matter(text)
        title = fm.get('title', '').strip()
        h1 = first_h1(text)
        if title and h1 and title != h1:
            bad.append((pp, title, h1))
    if bad:
        print('Title/H1 mismatches:')
        for pp, t, h in bad:
            print(f"- {pp}: title='{t}' != H1='{h}'")
        sys.exit(1)


if __name__ == '__main__':
    main()
