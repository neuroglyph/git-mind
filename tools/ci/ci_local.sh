#!/usr/bin/env bash
set -euo pipefail

# Local CI rig mirroring the C-Core Gate workflow
# - Markdown lint + docs checks
# - Optionally build and tidy on host (if requested)
# - Dockerized build + unit tests + E2E using the CI image

ROOT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
cd "$ROOT_DIR"

cleanup() {
  if [ -n "${CI_LOCAL_STAGE_DIR:-}" ] && [ -d "$CI_LOCAL_STAGE_DIR" ]; then
    rm -rf "$CI_LOCAL_STAGE_DIR"
  fi
}
trap cleanup EXIT

echo "==> Docs checks (frontmatter + links + TOC)"
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

if [ ! -f .ci/Dockerfile ]; then
  echo "❌ Missing .ci/Dockerfile (expected committed CI recipe)."
  echo "   Regenerate via ./tools/docker-clang-tidy.sh or restore from git."
  exit 1
fi

echo "==> Ensuring CI image is available locally"
if ! docker image inspect "$IMAGE" >/dev/null 2>&1; then
  echo "   Building $IMAGE from .ci/Dockerfile ..."
  docker build -t "$IMAGE" --label com.gitmind.project=git-mind -f .ci/Dockerfile .ci
fi

echo "==> Staging workspace snapshot for container run"
CI_LOCAL_STAGE_DIR=$(mktemp -d "${TMPDIR:-/tmp}/gitmind-ci-XXXXXX")
WORKSPACE_COPY="$CI_LOCAL_STAGE_DIR/workspace"
mkdir -p "$WORKSPACE_COPY"

PREV_CLANG_TIDY=""
if [ -f "$ROOT_DIR/clang-tidy-report.txt" ]; then
  PREV_CLANG_TIDY="$CI_LOCAL_STAGE_DIR/prev-clang-tidy-report.txt"
  cp "$ROOT_DIR/clang-tidy-report.txt" "$PREV_CLANG_TIDY"
fi

if command -v rsync >/dev/null 2>&1; then
  rsync -a --delete \
    --exclude '/build-local/' \
    --exclude '/ci_logs.zip' \
    --exclude '/clang-tidy-report.txt' \
    --exclude '/clang-tidy-report-full.txt' \
    "$ROOT_DIR"/ "$WORKSPACE_COPY"/
else
  echo "   rsync not available; falling back to tar copy"
  (cd "$ROOT_DIR" && tar -cf - . --exclude='./build-local' \
      --exclude='./ci_logs.zip' \
      --exclude='./clang-tidy-report.txt' \
      --exclude='./clang-tidy-report-full.txt') | (cd "$WORKSPACE_COPY" && tar -xf -)
fi

echo "==> Containerized build + tests + E2E"
set +e
docker run --rm --label com.gitmind.project=git-mind \
  -u "$(id -u):$(id -g)" \
  -v "$WORKSPACE_COPY":/workspace -w /workspace \
  -e GITMIND_DOCKER=1 -e GITMIND_SAFETY=off "$IMAGE" bash -lc '
    set -euo pipefail
    BUILD_DIR=/workspace/build-local
    if [ -d "$BUILD_DIR" ]; then
      CC=clang meson setup --reconfigure "$BUILD_DIR"
    else
      CC=clang meson setup "$BUILD_DIR"
    fi
    ninja -C "$BUILD_DIR" test

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
status=$?
set -e

echo "==> Collecting container artifacts"
for artifact in clang-tidy-report.txt clang-tidy-report-full.txt compile_commands.json; do
  if [ -f "$WORKSPACE_COPY/$artifact" ]; then
    cp "$WORKSPACE_COPY/$artifact" "$ROOT_DIR/$artifact"
  fi
done
if [ -f "$WORKSPACE_COPY/build-local/compile_commands.json" ]; then
  cp "$WORKSPACE_COPY/build-local/compile_commands.json" "$ROOT_DIR/compile_commands.json"
fi

if [ -n "$PREV_CLANG_TIDY" ] && [ -f "$ROOT_DIR/clang-tidy-report.txt" ]; then
  if ! diff -u "$PREV_CLANG_TIDY" "$ROOT_DIR/clang-tidy-report.txt" > "$ROOT_DIR/clang-tidy-report.diff"; then
    echo "clang-tidy diff written to clang-tidy-report.diff"
  else
    rm -f "$ROOT_DIR/clang-tidy-report.diff"
    echo "clang-tidy report unchanged"
  fi
fi

[ $status -eq 0 ] || exit $status

echo "✅ Local CI completed"
