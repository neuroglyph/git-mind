#!/usr/bin/env python3
"""
Apply feedback from a seeded review worksheet to GitHub:
- For each feedback item, detect Accepted/Rejected.
- Extract the original comment URL from the _Meta_ line.
- Post a reply to the original comment thread:
  - Accepted: brief acknowledgement, optional "Resolved" marker.
  - Rejected: paste the rejection block body.

Limitations:
- Resolving review threads programmatically typically requires GraphQL; this
  script currently posts replies only. Future work: add GraphQL mutation to
  resolve threads for Accepted items.

Usage (CI):
  python3 tools/review/apply_feedback_to_github.py --owner <o> --repo <r> <files...>

Env:
  GITHUB_TOKEN required.
"""
import argparse
import json
import os
import re
import sys
import urllib.request
import urllib.error

API = "https://api.github.com"


def gh_request(method: str, path: str, token: str, payload=None):
    url = API + path
    data = None
    headers = {
        "Accept": "application/vnd.github+json",
    }
    if token:
        headers["Authorization"] = f"Bearer {token}"
    if payload is not None:
        data = json.dumps(payload).encode("utf-8")
        headers["Content-Type"] = "application/json"
    req = urllib.request.Request(url, data=data, method=method, headers=headers)
    try:
        with urllib.request.urlopen(req) as r:
            body = r.read().decode("utf-8")
            return json.loads(body) if body else {}
    except urllib.error.HTTPError as e:
        msg = e.read().decode("utf-8", errors="ignore")
        print(f"GitHub API {method} {path} failed: {e.code} {msg}", file=sys.stderr)
        return None


SECTION_RE = re.compile(r"^###\s+(.*)$", re.MULTILINE)
META_RE = re.compile(r"^_Meta_:\s+(\S+)$", re.MULTILINE)
ACCEPT_RE = re.compile(r"^>\s*\[!note\]\-\s*\*\*Accepted\*\*", re.IGNORECASE | re.MULTILINE)
REJECT_RE = re.compile(r"^>\s*\[!CAUTION\]\-\s*\*\*Rejected\*\*", re.IGNORECASE | re.MULTILINE)


def extract_sections(text: str):
    """Yield (title, body) for each ### section until next ### or end."""
    titles = list(SECTION_RE.finditer(text))
    for idx, m in enumerate(titles):
        start = m.end()
        end = titles[idx + 1].start() if idx + 1 < len(titles) else len(text)
        yield (m.group(1).strip(), text[start:end].strip())


def parse_comment_meta(body: str):
    mm = META_RE.search(body)
    return mm.group(1).strip() if mm else ""


def accepted_or_rejected(body: str):
    is_acc = bool(ACCEPT_RE.search(body))
    is_rej = bool(REJECT_RE.search(body))
    if is_acc and not is_rej:
        return "accepted"
    if is_rej and not is_acc:
        return "rejected"
    return "unknown"


def reply_body(state: str, section_body: str):
    if state == "accepted":
        return "Acknowledged. Addressed as accepted in the worksheet. Marking as resolved. ✅"
    if state == "rejected":
        # Extract the rejection block and use it as reply body
        # Fallback to a short note if not found.
        m = REJECT_RE.search(section_body)
        if m:
            # Include from the rejection marker to next blank line group or end
            start = m.start()
            return section_body[start:].strip()
        return "Rejected — see worksheet for rationale and tradeoffs."
    return "Not classified — please review the worksheet section."


def handle_comment(owner: str, repo: str, token: str, meta_url: str, state: str, body: str):
    if "/pull/" in meta_url and "/review_comment/" in meta_url:
        # Code review comment URL -> extract comment id
        try:
            comment_id = int(meta_url.rsplit("/review_comment/", 1)[1].split("#")[0])
        except Exception:
            return False
        payload = {"body": reply_body(state, body)}
        resp = gh_request("POST", f"/repos/{owner}/{repo}/pulls/comments/{comment_id}/replies", token, payload)
        return resp is not None
    if "/issues/" in meta_url and "issuecomment-" in meta_url:
        # PR issue comment -> post a new issue comment to the PR
        try:
            pr_num = int(meta_url.split("/issues/")[1].split("#", 1)[0])
        except Exception:
            return False
        payload = {"body": reply_body(state, body)}
        resp = gh_request("POST", f"/repos/{owner}/{repo}/issues/{pr_num}/comments", token, payload)
        return resp is not None
    return False


def process_file(owner: str, repo: str, token: str, path: str):
    text = open(path, "r", encoding="utf-8").read()
    any_ok = False
    for title, body in extract_sections(text):
        meta = parse_comment_meta(body)
        if not meta:
            continue
        state = accepted_or_rejected(body)
        if state == "unknown":
            continue
        ok = handle_comment(owner, repo, token, meta, state, body)
        any_ok = any_ok or ok
    return any_ok


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--owner", required=True)
    ap.add_argument("--repo", required=True)
    ap.add_argument("files", nargs="+")
    args = ap.parse_args()

    token = os.environ.get("GITHUB_TOKEN", "")
    if not token:
        print("GITHUB_TOKEN is required", file=sys.stderr)
        sys.exit(1)

    status = 0
    for f in args.files:
        if not f.endswith(“.md”) or "docs/code-reviews/PR" not in f:
            continue
        ok = process_file(args.owner, args.repo, token, f)
        status |= 0 if ok else 0
    sys.exit(status)


if __name__ == "__main__":
    main()

