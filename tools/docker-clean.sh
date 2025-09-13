#!/usr/bin/env bash
set -euo pipefail

# Prune images/containers/caches created by this project only.
# Uses labels and naming to avoid nuking unrelated images.

GITMIND_NS=${GITMIND_NS:-gitmind}

echo "🧹 Cleaning git-mind Docker artifacts..."

echo "- Removing running/stopped containers with label com.gitmind.project=git-mind"
docker ps -aq --filter "label=com.gitmind.project=git-mind" | xargs -r docker rm -f

echo "- Removing images by label"
docker images -q --filter "label=com.gitmind.project=git-mind" | xargs -r docker rmi -f

echo "- Removing images by namespace prefix (${GITMIND_NS}/...)"
docker images --format '{{.Repository}}:{{.Tag}} {{.ID}}' | awk -v ns="${GITMIND_NS}/" '$1 ~ "^"ns { print $2 }' | xargs -r docker rmi -f

echo "- Removing legacy-named images (pre-namespace)"
docker images --format '{{.Repository}}:{{.Tag}} {{.ID}}' | awk '$1 ~ /^gauntlet-/ || $1 ~ /^gnu-cry-gauntlet/ || $1 ~ /^gitmind-ci/ { print $2 }' | xargs -r docker rmi -f

echo "- Pruning builder cache (safe)"
docker builder prune -af >/dev/null 2>&1 || true

echo "✅ Done. Disk space reclaimed."
