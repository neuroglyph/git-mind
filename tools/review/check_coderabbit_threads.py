#!/usr/bin/env python3
"""Check CodeRabbit review threads for unresolved feedback.

Usage:
  python3 tools/review/check_coderabbit_threads.py --owner <owner> --repo <repo> --pr <number>

Exits with status 0 when no unresolved CodeRabbit threads remain, otherwise prints
summary and exits with status 1.
"""
from __future__ import annotations

import argparse
import os
import sys
from typing import List, Tuple

import requests

GRAPHQL_ENDPOINT = "https://api.github.com/graphql"
CODERABBIT_LOGIN = "coderabbitai"


def github_graphql(token: str, query: str, variables: dict) -> dict:
    response = requests.post(
        GRAPHQL_ENDPOINT,
        json={"query": query, "variables": variables},
        headers={"Authorization": f"Bearer {token}", "Accept": "application/vnd.github+json"},
        timeout=30,
    )
    if response.status_code != 200:
        raise RuntimeError(
            f"GitHub GraphQL API returned {response.status_code}: {response.text}"
        )
    data = response.json()
    if "errors" in data:
        raise RuntimeError(f"GitHub GraphQL error: {data['errors']}")
    return data["data"]


def collect_unresolved_threads(token: str, owner: str, repo: str, number: int) -> List[Tuple[str, str]]:
    query = """
    query($owner: String!, $repo: String!, $number: Int!, $after: String) {
      repository(owner: $owner, name: $repo) {
        pullRequest(number: $number) {
          reviewThreads(first: 100, after: $after) {
            pageInfo { hasNextPage endCursor }
            nodes {
              isResolved
              comments(last: 1) {
                nodes {
                  author { login }
                  url
                  body
                }
              }
            }
          }
        }
      }
    }
    """

    unresolved: List[Tuple[str, str]] = []
    after = None
    while True:
        variables = {
            "owner": owner,
            "repo": repo,
            "number": number,
            "after": after,
        }
        data = github_graphql(token, query, variables)
        pr = data.get("repository", {}).get("pullRequest")
        if pr is None:
            raise RuntimeError(
                f"Pull request #{number} not found in {owner}/{repo}."
            )
        threads = pr["reviewThreads"]
        for thread in threads["nodes"]:
            if thread["isResolved"]:
                continue
            comments = thread["comments"]["nodes"]
            if not comments:
                continue
            last_comment = comments[-1]
            author = last_comment.get("author")
            login = author.get("login") if author else None
            if login == CODERABBIT_LOGIN:
                unresolved.append((last_comment.get("url", ""), last_comment.get("body", "")))
        page_info = threads["pageInfo"]
        if page_info["hasNextPage"]:
            after = page_info["endCursor"]
        else:
            break
    return unresolved


def main() -> int:
    parser = argparse.ArgumentParser(description="Check unresolved CodeRabbit threads")
    parser.add_argument("--owner", required=True)
    parser.add_argument("--repo", required=True)
    parser.add_argument("--pr", required=True, type=int)
    args = parser.parse_args()

    token = os.environ.get("GITHUB_TOKEN")
    if not token:
        print("GITHUB_TOKEN environment variable must be set", file=sys.stderr)
        return 1

    unresolved = collect_unresolved_threads(token, args.owner, args.repo, args.pr)

    if not unresolved:
        print("No unresolved CodeRabbit threads detected.")
        return 0

    print("Unresolved CodeRabbit threads detected:")
    for url, body in unresolved:
        snippet = body.strip().splitlines()[0] if body else ""
        print(f"- {url} :: {snippet}")
    print(
        "Resolve the threads or reply via the worksheet before marking the PR complete.",
        file=sys.stderr,
    )
    return 1


if __name__ == "__main__":
    sys.exit(main())
