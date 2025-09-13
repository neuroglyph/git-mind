#!/usr/bin/env bash
set -euo pipefail

# Print files touched by a Task:GM in repo 'git-mind' (default).
# Usage:
#   scripts/neo4j-show-task.sh <task-id> [repo]
#   GM_REPO=other-repo scripts/neo4j-show-task.sh <task-id>

if [ $# -lt 1 ]; then
  echo "Usage: $0 <task-id> [repo]" >&2
  exit 2
fi

TASK_ID="$1"
REPO="${2:-${GM_REPO:-git-mind}}"

TMP=$(mktemp)
cat >"$TMP" <<'JSON'
{
  "statements": [
    {
      "statement": "MATCH (t:Task:GM {id:$id, repo:$repo})-[:TOUCHES]->(f:File:GM {repo:$repo}) RETURN f.path AS path ORDER BY path",
      "parameters": {"id": "__TASK_ID__", "repo": "__REPO__"}
    }
  ]
}
JSON

sed -i.bak -e "s/__TASK_ID__/${TASK_ID//\//\\/}/" -e "s/__REPO__/${REPO//\//\\/}/" "$TMP" && rm -f "$TMP.bak"

OUT=$(bash "$(dirname "$0")/neo4j-curl.sh" -f "$TMP")
rm -f "$TMP"

# If jq is available, pretty-print the paths; else, raw JSON
if command -v jq >/dev/null 2>&1; then
  echo "$OUT" | jq -r '.results[0].data[]?.row[0]'
else
  echo "$OUT"
fi
