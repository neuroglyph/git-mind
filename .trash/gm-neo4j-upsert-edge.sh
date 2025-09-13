#!/usr/bin/env bash
set -euo pipefail

# Upsert a git-mind edge into Neo4j as a relationship between File:GM nodes.
# Computes blob OIDs at a specific commit and records full attribution.
#
# Usage:
#   scripts/gm-neo4j-upsert-edge.sh \
#     --commit <commitish> \
#     --src <path> --tgt <path> \
#     --type <REL_TYPE> [--lane <lane>] [--confidence <0..100>] \
#     [--author <email>] [--source <human|ai|other>] [--session <id>] \
#     [--ulid <ulid>] [--repo <git-mind>]
#
# Requires: git, bash, python3 (for base32 ULID-like generation), and scripts/neo4j-curl.sh

REPO="git-mind"
LANE="default"
CONF="100"
AUTHOR=""
SOURCE_TYPE="human"
SESSION_ID=""
ULID=""
COMMIT=""
SRC=""
TGT=""
REL_TYPE=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    --repo) REPO="$2"; shift 2 ;;
    --commit) COMMIT="$2"; shift 2 ;;
    --src) SRC="$2"; shift 2 ;;
    --tgt) TGT="$2"; shift 2 ;;
    --type) REL_TYPE="$2"; shift 2 ;;
    --lane) LANE="$2"; shift 2 ;;
    --confidence) CONF="$2"; shift 2 ;;
    --author) AUTHOR="$2"; shift 2 ;;
    --source) SOURCE_TYPE="$2"; shift 2 ;;
    --session) SESSION_ID="$2"; shift 2 ;;
    --ulid) ULID="$2"; shift 2 ;;
    -h|--help)
      sed -n '1,40p' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
    *) echo "Unknown arg: $1" >&2; exit 2 ;;
  esac
done

if [[ -z "$COMMIT" || -z "$SRC" || -z "$TGT" || -z "$REL_TYPE" ]]; then
  echo "Missing required args. See --help" >&2
  exit 2
fi

# Resolve commit SHA upfront
COMMIT_SHA=$(git rev-parse "$COMMIT")

# Gather commit metadata (author/committer)
COMMIT_INFO=$(git show -s --format='%H|%an|%ae|%at|%cn|%ce|%ct' "$COMMIT_SHA")
IFS='|' read -r _ COMMIT_AUTHOR_NAME COMMIT_AUTHOR_EMAIL COMMIT_AUTHOR_TIME COMMIT_COMMITTER_NAME COMMIT_COMMITTER_EMAIL COMMIT_COMMITTER_TIME <<EOF
$COMMIT_INFO
EOF

# Gather per-file last-change metadata at/before COMMIT
SRC_LAST_COMMIT=$(git rev-list -1 "$COMMIT_SHA" -- "$SRC" || true)
TGT_LAST_COMMIT=$(git rev-list -1 "$COMMIT_SHA" -- "$TGT" || true)

if [[ -n "$SRC_LAST_COMMIT" ]]; then
  SRC_INFO=$(git show -s --format='%an|%ae|%at' "$SRC_LAST_COMMIT")
  IFS='|' read -r SRC_CODE_AUTHOR_NAME SRC_CODE_AUTHOR_EMAIL SRC_CODE_AUTHOR_TIME <<EOF
$SRC_INFO
EOF
else
  SRC_CODE_AUTHOR_NAME=""
  SRC_CODE_AUTHOR_EMAIL=""
  SRC_CODE_AUTHOR_TIME="0"
fi

if [[ -n "$TGT_LAST_COMMIT" ]]; then
  TGT_INFO=$(git show -s --format='%an|%ae|%at' "$TGT_LAST_COMMIT")
  IFS='|' read -r TGT_CODE_AUTHOR_NAME TGT_CODE_AUTHOR_EMAIL TGT_CODE_AUTHOR_TIME <<EOF
$TGT_INFO
EOF
else
  TGT_CODE_AUTHOR_NAME=""
  TGT_CODE_AUTHOR_EMAIL=""
  TGT_CODE_AUTHOR_TIME="0"
fi

# Resolve blob OIDs for src/tgt at COMMIT
SRC_SHA=$(git ls-tree -r "$COMMIT_SHA" -- "$SRC" | awk '{print $3}' | head -n1)
TGT_SHA=$(git ls-tree -r "$COMMIT_SHA" -- "$TGT" | awk '{print $3}' | head -n1)

if [[ -z "$SRC_SHA" ]]; then echo "Could not resolve src blob OID for $SRC at $COMMIT" >&2; exit 1; fi
if [[ -z "$TGT_SHA" ]]; then echo "Could not resolve tgt blob OID for $TGT at $COMMIT" >&2; exit 1; fi

# Default author from git config if not provided
if [[ -z "$AUTHOR" ]]; then
  GIT_EMAIL=$(git config --get user.email || true)
  GIT_NAME=$(git config --get user.name || true)
  if [[ -n "$GIT_NAME" && -n "$GIT_EMAIL" ]]; then
    AUTHOR="$GIT_NAME <$GIT_EMAIL>"
  elif [[ -n "$GIT_EMAIL" ]]; then
    AUTHOR="$GIT_EMAIL"
  elif [[ -n "$GIT_NAME" ]]; then
    AUTHOR="$GIT_NAME"
  else
    AUTHOR="user@local"
  fi
fi

# Generate a ULID-like id if not provided (26-char base32 from ms timestamp + random)
if [[ -z "$ULID" ]]; then
  ULID=$(python3 - "$COMMIT" <<'PY'
import base64, os, sys, time
t = int(time.time()*1000).to_bytes(6,'big')
r = os.urandom(10)
print(base64.b32encode(t+r).decode().rstrip('='))
PY
)
fi

TMP=$(mktemp)
cat >"$TMP" <<'JSON'
{
  "statements": [
    {"statement":"MERGE (a:File:GM {path:$a_path}) ON CREATE SET a.repo=$repo ON MATCH SET a.repo=$repo",
     "parameters":{"a_path":"__SRC__","repo":"__REPO__"}},
    {"statement":"MERGE (b:File:GM {path:$b_path}) ON CREATE SET b.repo=$repo ON MATCH SET b.repo=$repo",
     "parameters":{"b_path":"__TGT__","repo":"__REPO__"}},
    {"statement":"MATCH (a:File:GM {path:$a_path, repo:$repo}),(b:File:GM {path:$b_path, repo:$repo}) MERGE (a)-[e:EDGE {ulid:$ulid}]->(b) ON CREATE SET e.repo=$repo, e.type=$rel_type, e.lane=$lane, e.confidence=toInteger($conf), e.ts=datetime(), e.commit_ref=$commit_ref, e.commit_sha=$commit_sha, e.src_path=$a_path, e.tgt_path=$b_path, e.src_sha=$src_sha, e.tgt_sha=$tgt_sha, e.author=$author, e.source_type=$source_type, e.session_id=$session_id, e.commit_author_name=$commit_author_name, e.commit_author_email=$commit_author_email, e.commit_author_time=toInteger($commit_author_time), e.commit_committer_name=$commit_committer_name, e.commit_committer_email=$commit_committer_email, e.commit_committer_time=toInteger($commit_committer_time), e.src_last_commit=$src_last_commit, e.src_code_author_name=$src_code_author_name, e.src_code_author_email=$src_code_author_email, e.src_code_author_time=toInteger($src_code_author_time), e.tgt_last_commit=$tgt_last_commit, e.tgt_code_author_name=$tgt_code_author_name, e.tgt_code_author_email=$tgt_code_author_email, e.tgt_code_author_time=toInteger($tgt_code_author_time) ON MATCH SET e.type=$rel_type, e.lane=$lane, e.confidence=toInteger($conf), e.commit_ref=$commit_ref, e.commit_sha=$commit_sha, e.src_sha=$src_sha, e.tgt_sha=$tgt_sha, e.author=$author, e.source_type=$source_type, e.session_id=$session_id, e.commit_author_name=$commit_author_name, e.commit_author_email=$commit_author_email, e.commit_author_time=toInteger($commit_author_time), e.commit_committer_name=$commit_committer_name, e.commit_committer_email=$commit_committer_email, e.commit_committer_time=toInteger($commit_committer_time), e.src_last_commit=$src_last_commit, e.src_code_author_name=$src_code_author_name, e.src_code_author_email=$src_code_author_email, e.src_code_author_time=toInteger($src_code_author_time), e.tgt_last_commit=$tgt_last_commit, e.tgt_code_author_name=$tgt_code_author_name, e.tgt_code_author_email=$tgt_code_author_email, e.tgt_code_author_time=toInteger($tgt_code_author_time)",
     "parameters":{"a_path":"__SRC__","b_path":"__TGT__","repo":"__REPO__","ulid":"__ULID__","rel_type":"__REL_TYPE__","lane":"__LANE__","conf":"__CONF__","commit_ref":"__COMMIT__","commit_sha":"__COMMIT_SHA__","src_sha":"__SRC_SHA__","tgt_sha":"__TGT_SHA__","author":"__AUTHOR__","source_type":"__SOURCE_TYPE__","session_id":"__SESSION_ID__","commit_author_name":"__COMMIT_AUTHOR_NAME__","commit_author_email":"__COMMIT_AUTHOR_EMAIL__","commit_author_time":"__COMMIT_AUTHOR_TIME__","commit_committer_name":"__COMMIT_COMMITTER_NAME__","commit_committer_email":"__COMMIT_COMMITTER_EMAIL__","commit_committer_time":"__COMMIT_COMMITTER_TIME__","src_last_commit":"__SRC_LAST_COMMIT__","src_code_author_name":"__SRC_CODE_AUTHOR_NAME__","src_code_author_email":"__SRC_CODE_AUTHOR_EMAIL__","src_code_author_time":"__SRC_CODE_AUTHOR_TIME__","tgt_last_commit":"__TGT_LAST_COMMIT__","tgt_code_author_name":"__TGT_CODE_AUTHOR_NAME__","tgt_code_author_email":"__TGT_CODE_AUTHOR_EMAIL__","tgt_code_author_time":"__TGT_CODE_AUTHOR_TIME__"}}
  ]
}
JSON

sed -i.bak \
  -e "s#__SRC__#${SRC//\//\\/}#g" \
  -e "s#__TGT__#${TGT//\//\\/}#g" \
  -e "s#__REPO__#${REPO//\//\\/}#g" \
  -e "s#__ULID__#${ULID//\//\\/}#g" \
  -e "s#__REL_TYPE__#${REL_TYPE//\//\\/}#g" \
  -e "s#__LANE__#${LANE//\//\\/}#g" \
  -e "s#__CONF__#${CONF//\//\\/}#g" \
  -e "s#__COMMIT__#${COMMIT//\//\\/}#g" \
  -e "s#__COMMIT_SHA__#${COMMIT_SHA//\//\\/}#g" \
  -e "s#__SRC_SHA__#${SRC_SHA//\//\\/}#g" \
  -e "s#__TGT_SHA__#${TGT_SHA//\//\\/}#g" \
  -e "s#__AUTHOR__#${AUTHOR//\//\\/}#g" \
  -e "s#__SOURCE_TYPE__#${SOURCE_TYPE//\//\\/}#g" \
  -e "s#__SESSION_ID__#${SESSION_ID//\//\\/}#g" \
  -e "s#__COMMIT_AUTHOR_NAME__#${COMMIT_AUTHOR_NAME//\//\\/}#g" \
  -e "s#__COMMIT_AUTHOR_EMAIL__#${COMMIT_AUTHOR_EMAIL//\//\\/}#g" \
  -e "s#__COMMIT_AUTHOR_TIME__#${COMMIT_AUTHOR_TIME//\//\\/}#g" \
  -e "s#__COMMIT_COMMITTER_NAME__#${COMMIT_COMMITTER_NAME//\//\\/}#g" \
  -e "s#__COMMIT_COMMITTER_EMAIL__#${COMMIT_COMMITTER_EMAIL//\//\\/}#g" \
  -e "s#__COMMIT_COMMITTER_TIME__#${COMMIT_COMMITTER_TIME//\//\\/}#g" \
  -e "s#__SRC_LAST_COMMIT__#${SRC_LAST_COMMIT//\//\\/}#g" \
  -e "s#__SRC_CODE_AUTHOR_NAME__#${SRC_CODE_AUTHOR_NAME//\//\\/}#g" \
  -e "s#__SRC_CODE_AUTHOR_EMAIL__#${SRC_CODE_AUTHOR_EMAIL//\//\\/}#g" \
  -e "s#__SRC_CODE_AUTHOR_TIME__#${SRC_CODE_AUTHOR_TIME//\//\\/}#g" \
  -e "s#__TGT_LAST_COMMIT__#${TGT_LAST_COMMIT//\//\\/}#g" \
  -e "s#__TGT_CODE_AUTHOR_NAME__#${TGT_CODE_AUTHOR_NAME//\//\\/}#g" \
  -e "s#__TGT_CODE_AUTHOR_EMAIL__#${TGT_CODE_AUTHOR_EMAIL//\//\\/}#g" \
  -e "s#__TGT_CODE_AUTHOR_TIME__#${TGT_CODE_AUTHOR_TIME//\//\\/}#g" "$TMP" && rm -f "$TMP.bak"

echo "Upserting EDGE $REL_TYPE ($SRC -> $TGT) at $COMMIT [ulid=$ULID]"
bash "$(dirname "$0")/neo4j-curl.sh" -f "$TMP" >/dev/null
rm -f "$TMP"
echo "✅ Done"
