#!/usr/bin/env python3
import argparse
import os
import subprocess
import re
import sys
from typing import List, Tuple


LINK_PATTERN = re.compile(r"!?\[([^\]]+)\]\(([^)]+)\)")


SKIP_DIRS = {'.git', '.legacy', '.trash'}


REQUIRE_TOC_PREFIXES = (
    os.path.join('docs', 'planning') + os.sep,
    os.path.join('docs', 'specs') + os.sep,
    os.path.join('docs', 'testing') + os.sep,
    os.path.join('docs', 'deployment') + os.sep,
    os.path.join('docs', 'requirements') + os.sep,
    os.path.join('docs', 'risk') + os.sep,
    os.path.join('docs', 'charter') + os.sep,
    os.path.join('docs', 'quality') + os.sep,
    os.path.join('docs', 'PRDs') + os.sep,
    os.path.join('docs', 'adr') + os.sep,
    os.path.join('docs', 'architecture') + os.sep,
    os.path.join('docs', 'cli') + os.sep,
)

REQUIRE_TOC_EXACT = {os.path.join('docs', 'architecture', 'System_Architecture.md')}


def is_required_toc(path: str) -> bool:
    norm = path.replace('\\', '/')
    if norm in REQUIRE_TOC_EXACT:
        return True
    return any(norm.startswith(p.replace('\\', '/')) for p in REQUIRE_TOC_PREFIXES)


def is_git_ignored(path: str) -> bool:
    try:
        # Return True if git says the path is ignored
        res = subprocess.run(
            ["git", "check-ignore", "-q", path],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )
        return res.returncode == 0
    except Exception:
        # If git is unavailable, don't treat as ignored
        return False


def find_md_files(base: str = 'docs') -> List[str]:
    out = []
    for root, dirs, files in os.walk(base):
        # prune skip dirs
        dirs[:] = [d for d in dirs if d not in SKIP_DIRS]
        for f in files:
            if f.lower().endswith('.md'):
                candidate = os.path.join(root, f)
                if not is_git_ignored(candidate):
                    out.append(candidate)
    return out


def check_links(files: List[str]) -> List[str]:
    errors: List[str] = []
    for path in files:
        skip = any(seg in SKIP_DIRS for seg in path.split(os.sep))
        if skip:
            continue
        try:
            with open(path, 'r', encoding='utf-8') as fh:
                for idx, line in enumerate(fh, start=1):
                    for m in LINK_PATTERN.finditer(line):
                        link = m.group(2).strip()
                        if not link or link.startswith('#'):
                            continue
                        if '://' in link or link.startswith('mailto:') or link.startswith('tel:'):
                            continue
                        # strip anchors
                        link_path = link.split('#', 1)[0]
                        # ignore absolute URLs and absolute file paths
                        if os.path.isabs(link_path):
                            continue
                        target = os.path.normpath(os.path.join(os.path.dirname(path), link_path))
                        if not os.path.exists(target):
                            errors.append(f"{path}:{idx}: broken link → {link}")
        except Exception as e:
            errors.append(f"{path}:0: exception reading file: {e}")
    return errors


def check_toc(files: List[str]) -> List[str]:
    errors: List[str] = []
    for path in files:
        skip = any(seg in SKIP_DIRS for seg in path.split(os.sep))
        if skip:
            continue
        if not is_required_toc(path):
            continue
        try:
            with open(path, 'r', encoding='utf-8') as fh:
                content = fh.read()
                if 'Table of Contents' not in content:
                    errors.append(f"{path}: missing 'Table of Contents' section")
        except Exception as e:
            errors.append(f"{path}: exception reading file: {e}")
    return errors


def main(argv: List[str]) -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument('--mode', choices=['link', 'toc'], required=True)
    ap.add_argument('files', nargs='*')
    args = ap.parse_args(argv)

    files = [f for f in args.files if f.endswith('.md') and not is_git_ignored(f)]
    if not files:
        files = find_md_files('docs')

    if args.mode == 'link':
        errs = check_links(files)
    else:
        errs = check_toc(files)

    if errs:
        print('\n'.join(errs))
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
