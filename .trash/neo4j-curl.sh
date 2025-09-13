#!/usr/bin/env bash
set -euo pipefail

# Simple Neo4j HTTP helper.
# Reads JSON from stdin or -f <file> and POSTs to Neo4j transactional endpoint.

NEO4J_HTTP_URL=${NEO4J_HTTP_URL:-http://localhost:7474}
NEO4J_USER=${NEO4J_USER:-neo4j}
NEO4J_PASSWORD=${NEO4J_PASSWORD:-password123}

FILE=""
while getopts ":f:" opt; do
  case ${opt} in
    f) FILE="$OPTARG" ;;
    :) echo "Option -$OPTARG requires an argument" >&2; exit 2 ;;
    \?) echo "Usage: $0 [-f payload.json] < payload.json" >&2; exit 2 ;;
  esac
done
shift $((OPTIND-1))

TMP=""
cleanup() { [ -n "$TMP" ] && rm -f "$TMP" || true; }
trap cleanup EXIT

if [ -n "$FILE" ]; then
  PAYLOAD="@$FILE"
else
  TMP=$(mktemp)
  cat > "$TMP"
  PAYLOAD="@$TMP"
fi

curl -sS -u "$NEO4J_USER:$NEO4J_PASSWORD" \
  -H 'Content-Type: application/json' \
  -d "$PAYLOAD" \
  "$NEO4J_HTTP_URL/db/neo4j/tx/commit"

