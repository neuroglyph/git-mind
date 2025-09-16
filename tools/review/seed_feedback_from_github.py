#!/usr/bin/env python3
"""
Seed a Code Review Feedback doc from a GitHub PR.

Generates docs/code-reviews/PR<PR#>/<commit_sha>.md prefilled with
feedback items extracted from:
- PR review comments (code comments)
- PR issue comments (general discussion)

Authentication: set GITHUB_TOKEN in the environment.

Usage:
  python3 tools/review/seed_feedback_from_github.py \
    --owner neuroglyph --repo git-mind --pr 169 \
    [--commit <sha>] [--out docs/code-reviews]

If --commit is omitted, the PR head SHA is used.
"""

import argparse
import datetime as dt
import json
import os
import pathlib
import textwrap
import urllib.request
import urllib.error


API = "https://api.github.com"


def gh_get(path: str, token: str):
    req = urllib.request.Request(API + path)
    req.add_header("Accept", "application/vnd.github+json")
    if token:
        req.add_header("Authorization", f"Bearer {token}")
    try:
        with urllib.request.urlopen(req) as r:
            return json.loads(r.read().decode("utf-8"))
    except urllib.error.HTTPError as e:
        msg = e.read().decode("utf-8", errors="ignore")
        raise SystemExit(f"GitHub API error {e.code} on {path}: {msg}")


def normalize(s: str) -> str:
    return s.replace("\r\n", "\n").strip()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--owner", required=True)
    ap.add_argument("--repo", required=True)
    ap.add_argument("--pr", type=int, required=True)
    ap.add_argument("--commit", default=None, help="Target commit SHA (defaults to PR head)")
    ap.add_argument("--out", default="docs/code-reviews", help="Base output dir")
    args = ap.parse_args()

    token = os.environ.get("GITHUB_TOKEN", "")
    pr_path = f"/repos/{args.owner}/{args.repo}/pulls/{args.pr}"
    pr = gh_get(pr_path, token)
    head_sha = (args.commit or pr.get("head", {}).get("sha", "")).strip()
    head_ref = pr.get("head", {}).get("ref", "")
    pr_url = pr.get("html_url", "")

    if not head_sha:
        raise SystemExit("Unable to determine PR head SHA. Pass --commit explicitly.")

    # Collect review comments
    rev_comments = gh_get(pr_path + "/comments?per_page=100", token)
    # Collect issue comments (discussion)
    iss_comments = gh_get(f"/repos/{args.owner}/{args.repo}/issues/{args.pr}/comments?per_page=100", token)

    # Build output path
    out_dir = pathlib.Path(args.out) / f"PR{args.pr}"
    out_dir.mkdir(parents=True, exist_ok=True)
    out_file = out_dir / f"{head_sha}.md"

    # Header/front matter
    today = dt.datetime.utcnow().strftime("%Y-%m-%d")
    agent = "CodeRabbit (and reviewers)"

    def fm_line(k, v):
        return f"{k}: {v}\n"

    content = []
    content.append("---\n")
    content.append(fm_line("title", out_file.name))
    content.append("description: Preserved review artifacts and rationale.\n")
    content.append("audience: [contributors]\n")
    content.append("domain: [quality]\n")
    content.append("tags: [review]\n")
    content.append("status: archive\n")
    content.append("---\n\n")

    content.append("# Code Review Feedback\n\n")
    content.append("| Date | Agent | SHA | Branch | PR |\n")
    content.append("|------|-------|-----|--------|----|\n")
    content.append(f"| {today} | {agent} | `{head_sha}` | [{head_ref}](https://github.com/{args.owner}/{args.repo}/tree/{head_ref} \"{args.owner}/{args.repo}:{head_ref}\") | [PR#{args.pr}]({pr_url}) |\n\n")

    content.append("## CODE REVIEW FEEDBACK\n\n")

    # Helper: emit one feedback block
    def emit_feedback(title: str, body: str, meta: str = ""):
        content.append(f"### {title}\n\n")
        fence = "```text\n" + normalize(body) + "\n```\n\n"
        content.append(fence)
        if meta:
            content.append(f"_Meta_: {meta}\n\n")
        content.append("{response}\n\n")

    # Add review comments (code)
    for c in rev_comments:
        path = c.get("path", "")
        line = c.get("line") or c.get("original_line")
        url = c.get("html_url", "")
        user = c.get("user", {}).get("login", "")
        title = f"{path}:{line} — {user}"
        body = c.get("body", "")
        meta = url
        emit_feedback(title, body, meta)

    # Add issue comments (general)
    for c in iss_comments:
        body = c.get("body", "")
        user = c.get("user", {}).get("login", "")
        url = c.get("html_url", "")
        title = f"General comment — {user}"
        emit_feedback(title, body, url)

    with open(out_file, "w", encoding="utf-8") as f:
        f.write("".join(content))

    print(f"Wrote {out_file}")


if __name__ == "__main__":
    main()

