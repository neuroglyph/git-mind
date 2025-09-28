#!/usr/bin/env python3
"""
Fail if any review worksheets contain unfilled templates or placeholders.

This checker looks for:
- Presence of "{response}" placeholders.
- Presence of any "{placeholder}" tokens from the templates (e.g., {lesson}).
- Sections (### ...) that do not include an Accepted or Rejected marker.

Usage:
  python3 tools/review/check_worksheets.py <file1> <file2> ...

Exit codes:
  0 = OK or no relevant files provided
  1 = Found unaddressed placeholders or sections
"""
import re
import sys


PLACEHOLDER_PATTERNS = [
    r"\{response\}",
    r"\{confidence_score_out_of_10\}",
    r"\{confidence_rationale\}",
    r"\{lesson\}",
    r"\{what_you_did\}",
    r"\{regression_avoidance_strategy\}",
    r"\{any_additional_context_or_say_none\}",
    r"\{rationale\}",
    r"\{pros_and_cons\}",
    r"\{change_mind_conditions\}",
    r"\{future_plans\}",
    r"\{accepted_count\}",
    r"\{rejected_count\}",
    r"\{remarks\}",
    r"\{comments\}",
]

SECTION_RE = re.compile(r"^###\s+", re.MULTILINE)
ACCEPT_RE = re.compile(r"^>\s*\[!note\]\-\s*\*\*Accepted\*\*", re.IGNORECASE | re.MULTILINE)
REJECT_RE = re.compile(r"^>\s*\[!CAUTION\]\-\s*\*\*Rejected\*\*", re.IGNORECASE | re.MULTILINE)


def check_file(path: str) -> list[str]:
    problems = []
    try:
        text = open(path, "r", encoding="utf-8").read()
    except FileNotFoundError:
        # Deleted worksheets are acceptable; no further action required.
        return problems
    except Exception as e:
        problems.append(f"cannot read {path}: {e}")
        return problems

    # Check for placeholders
    for pat in PLACEHOLDER_PATTERNS:
        if re.search(pat, text):
            problems.append(f"{path}: contains unfilled placeholder '{pat}'")

    # Check every section has Accepted or Rejected
    sections = list(SECTION_RE.finditer(text))
    if sections:
        for idx, m in enumerate(sections):
            start = m.end()
            end = sections[idx + 1].start() if idx + 1 < len(sections) else len(text)
            body = text[start:end]
            if not (ACCEPT_RE.search(body) or REJECT_RE.search(body)):
                problems.append(f"{path}: section at offset {start} missing Accepted/Rejected decision")

    return problems


def main():
    if len(sys.argv) <= 1:
        sys.exit(0)
    files = [f for f in sys.argv[1:] if f.endswith('.md') and 'docs/code-reviews/PR' in f]
    if not files:
        sys.exit(0)
    all_problems = []
    for f in files:
        all_problems.extend(check_file(f))
    if all_problems:
        print("Review worksheet issues detected:")
        for p in all_problems:
            print("-", p)
        print("\nResolve all placeholders and mark each item Accepted or Rejected.\n"
              "If you must bypass temporarily: HOOKS_BYPASS=1 git push")
        sys.exit(1)
    sys.exit(0)


if __name__ == '__main__':
    main()
