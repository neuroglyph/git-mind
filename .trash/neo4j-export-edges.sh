#!/usr/bin/env bash
set -euo pipefail

# Export EDGE relationships for a repo to JSON (or NDJSON if jq present).
# Usage:
#   scripts/neo4j-export-edges.sh [repo]
# Default repo: git-mind

REPO="${1:-git-mind}"

TMP=$(mktemp)
cat >"$TMP" <<'JSON'
{
  "statements": [
    {
      "statement": "MATCH (a:File:GM {repo:$repo})-[e:EDGE {repo:$repo}]->(b:File:GM {repo:$repo}) RETURN a.path AS src_path, e AS edge, b.path AS tgt_path ORDER BY e.ts",
      "parameters": {"repo": "__REPO__"}
    }
  ]
}
JSON

sed -i.bak -e "s/__REPO__/${REPO//\//\\/}/" "$TMP" && rm -f "$TMP.bak"

OUT=$(bash "$(dirname "$0")/neo4j-curl.sh" -f "$TMP")
rm -f "$TMP"

if command -v jq >/dev/null 2>&1; then
  echo "$OUT" | jq -r '.results[0].data[] | {src_path:.row[0], edge:.row[1], tgt_path:.row[2]}'
else
  echo "$OUT"
fi
