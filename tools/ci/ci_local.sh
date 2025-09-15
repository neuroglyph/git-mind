#!/usr/bin/env bash
set -euo pipefail

# Local CI rig mirroring the C-Core Gate workflow
# - Markdown lint + docs checks
# - Optionally build and tidy on host (if requested)
# - Dockerized build + unit tests + E2E using the CI image

ROOT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
cd "$ROOT_DIR"

echo "==> Markdown lint + docs checks"
npx --yes -p markdownlint-cli2 markdownlint-cli2 "**/*.md" "#.git" "#build"
python3 tools/docs/check_frontmatter.py
python3 tools/docs/check_docs.py --mode link
python3 tools/docs/check_docs.py --mode toc

IMAGE="${GITMIND_CI_IMAGE:-gitmind/ci:clang-20}"
echo "==> Using CI image: $IMAGE"

if ! command -v docker >/dev/null 2>&1; then
  echo "⚠️ Docker not found. Falling back to host build (override guard)."
  export GITMIND_ALLOW_HOST_BUILD=1
  CC=${CC:-clang}
  echo "==> Host build with $CC"
  rm -rf build-local
  CC=$CC meson setup build-local
  ninja -C build-local
  ninja -C build-local test
  exit 0
fi

echo "==> Pulling CI image (best-effort)"
docker pull "$IMAGE" || true

echo "==> Containerized build + tests + E2E"
docker run --rm --label com.gitmind.project=git-mind \
  -u "$(id -u):$(id -g)" \
  -v "$PWD":/workspace -w /workspace \
  -e GITMIND_DOCKER=1 -e GITMIND_SAFETY=off "$IMAGE" bash -lc '
    set -euo pipefail
    BUILD_DIR=/workspace/build-local
    if [ -d "$BUILD_DIR" ]; then
      CC=clang meson setup --reconfigure "$BUILD_DIR"
    else
      CC=clang meson setup "$BUILD_DIR"
    fi
    ninja -C "$BUILD_DIR" git-mind
    export GIT_MIND="$BUILD_DIR/git-mind"
    ninja -C "$BUILD_DIR" test
    rm -rf /tmp/gm-e2e && mkdir -p /tmp/gm-e2e
    cp -r tests/e2e /tmp/gm-e2e/
    cd /tmp/gm-e2e/e2e
    bash ./run_all_tests.sh

    echo "==> clang-tidy (diff-guard style)"
    cd /workspace
    cp "$BUILD_DIR/compile_commands.json" . || true
    if command -v clang-tidy >/dev/null 2>&1; then
      clang-tidy -quiet -p . --config-file=quality/.clang-tidy \
        $(git ls-files "core/**/*.c" "core/**/*.h" | grep -v -E "(test_|_test\\.|/tests/|core/src/hooks/)") \
        | tee clang-tidy-report-full.txt || true
      grep -E "^/.*core/|^core/" clang-tidy-report-full.txt > clang-tidy-report.txt || true
      echo "clang-tidy finished (report: clang-tidy-report.txt)"
    else
      echo "clang-tidy not found in image; skipping"
    fi
  '

echo "✅ Local CI completed"

