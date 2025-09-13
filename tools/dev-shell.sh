#!/usr/bin/env bash
set -euo pipefail

GITMIND_NS=${GITMIND_NS:-gitmind}
CI_TAG=${CI_TAG:-clang-20}
IMAGE=${GITMIND_CI_IMAGE:-$GITMIND_NS/ci:$CI_TAG}

# Ensure CI image exists (reuse the same Dockerfile as clang-tidy helper)
if [ ! -f .ci/Dockerfile ]; then
  echo "Bootstrapping CI Dockerfile (clang-20, libroaring-dev, libgit2, libsodium)..."
  bash ./tools/docker-clang-tidy.sh >/dev/null 2>&1 || true
fi

if ! docker image inspect "$IMAGE" >/dev/null 2>&1; then
  echo "Building CI Docker image $IMAGE ..."
  docker build -t "$IMAGE" --label com.gitmind.project=git-mind .ci
fi

echo "Opening dev shell in Docker image $IMAGE ..."
docker run --rm -it \
  --label com.gitmind.project=git-mind \
  -e GITMIND_DOCKER=1 \
  -v "$PWD":/workspace -w /workspace \
  "$IMAGE" bash
