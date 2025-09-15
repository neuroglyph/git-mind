#!/usr/bin/env python3
import argparse
import datetime as dt
import os
import subprocess
import sys

CHANGELOG = os.path.join(os.path.dirname(os.path.dirname(os.path.dirname(__file__))), 'CHANGELOG.md')

def git_short_sha():
    try:
        out = subprocess.check_output(['git', 'rev-parse', '--short', 'HEAD'], stderr=subprocess.DEVNULL)
        return out.decode().strip()
    except Exception:
        return 'unknown'

def ensure_date_section(lines, date_str):
    header = f"## {date_str}\n"
    if any(l.strip() == header.strip() for l in lines):
        return lines
    # Insert date header under "## Daily Dev Log" section or append at end
    new_lines = []
    inserted = False
    in_daily = False
    for i, line in enumerate(lines):
        new_lines.append(line)
        if line.startswith('## Daily Dev Log'):
            in_daily = True
            continue
        if in_daily and not inserted:
            # next blank line is a good insertion point
            if line.strip() == '' and i + 1 < len(lines):
                new_lines.append(header)
                new_lines.append('\n')
                inserted = True
                in_daily = False
    if not inserted:
        if not new_lines or new_lines[-1].strip() != '':
            new_lines.append('\n')
        new_lines.append(header)
        new_lines.append('\n')
    return new_lines

def main():
    parser = argparse.ArgumentParser(description='Append a daily dev log entry to CHANGELOG.md')
    parser.add_argument('-m', '--message', required=True, help='One-liner description')
    parser.add_argument('-n', '--note', default=None, help='Optional note block to add')
    args = parser.parse_args()

    now = dt.datetime.now()
    date_str = now.strftime('%Y-%m-%d')
    time_str = now.strftime('%H:%M')
    sha = git_short_sha()

    if not os.path.exists(CHANGELOG):
        print(f"CHANGELOG not found at {CHANGELOG}", file=sys.stderr)
        return 1

    with open(CHANGELOG, 'r', encoding='utf-8') as f:
        lines = f.readlines()

    lines = ensure_date_section(lines, date_str)

    entry = f"- @ {time_str} – `{sha}` {args.message}\n"
    lines.append(entry)
    if args.note:
        lines.append('\n')
        lines.append('> [!note]\n')
        lines.append(f"> {args.note}\n")
        lines.append('\n')

    with open(CHANGELOG, 'w', encoding='utf-8') as f:
        f.writelines(lines)
    return 0

if __name__ == '__main__':
    sys.exit(main())

