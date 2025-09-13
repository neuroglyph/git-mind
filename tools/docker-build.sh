#!/usr/bin/env bash
set -euo pipefail

GITMIND_NS=${GITMIND_NS:-gitmind}
CI_TAG=${CI_TAG:-clang-20}
IMAGE=${GITMIND_CI_IMAGE:-$GITMIND_NS/ci:$CI_TAG}

bash ./tools/dev-shell.sh -c true >/dev/null 2>&1 || true
if ! docker image inspect "$IMAGE" >/dev/null 2>&1; then
  echo "Building CI Docker image..."
  docker build -t "$IMAGE" --label com.gitmind.project=git-mind .ci
fi

docker run --rm \
  --label com.gitmind.project=git-mind \
  -e GITMIND_DOCKER=1 \
  -v "$PWD":/workspace -w /workspace \
  "$IMAGE" bash -lc 'CC=clang meson setup build >/dev/null 2>&1 || true && ninja -C build'
