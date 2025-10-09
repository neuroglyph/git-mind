#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
DEFAULT_IMAGE=${GITMIND_DEBUG_IMAGE:-gitmind/ci:clang-20}
usage() {
  cat <<USAGE
usage: tools/debug/debug-container.sh [options] [-- <command>]

Stage the current workspace into an isolated directory, drop the origin remote,
then launch the git-mind CI Docker image for iterative debugging.

Options:
  -i, --image <name>       Docker image to use (default: ${DEFAULT_IMAGE})
  -s, --stage <dir>        Reuse an existing stage directory (must be writable)
  -k, --keep-stage         Keep the staged copy after the container exits
  -p, --packages <list>    Space-separated apt packages to install (e.g. "gdb lldb")
  -r, --resync             Re-rsync the repository into the stage directory before launch
  -h, --help               Show this help

Any arguments after '--' are executed inside the container; omit to open an interactive shell.
USAGE
}

IMAGE=${DEFAULT_IMAGE}
STAGE_DIR=${GITMIND_DEBUG_STAGE:-}
AUTO_STAGE=0
KEEP_STAGE=${GITMIND_DEBUG_KEEP_STAGE:-0}
INSTALL_PKGS=${GITMIND_DEBUG_PACKAGES:-}
RESYNC=${GITMIND_DEBUG_RESYNC:-0}
USER_CMD=()

while [[ $# -gt 0 ]]; do
  case "$1" in
    -i|--image)
      IMAGE="$2"; shift 2 ;;
    -s|--stage)
      STAGE_DIR="$2"; shift 2 ;;
    -k|--keep-stage)
      KEEP_STAGE=1; shift ;;
    -p|--packages)
      INSTALL_PKGS="$2"; shift 2 ;;
    -r|--resync)
      RESYNC=1; shift ;;
    -h|--help)
      usage; exit 0 ;;
    --)
      shift
      USER_CMD=("$@")
      break ;;
    *)
      echo "Unknown option: $1" >&2
      usage
      exit 1 ;;
  esac
done

if [[ -z "${STAGE_DIR}" ]]; then
  STAGE_DIR=$(mktemp -d "${TMPDIR:-/tmp}/gitmind-debug-XXXXXX")
  RESYNC=1
  AUTO_STAGE=1
else
  mkdir -p "${STAGE_DIR}"
fi

if [[ ${RESYNC} -eq 1 ]]; then
  echo "==> Syncing workspace into ${STAGE_DIR}" >&2
  rsync -a --delete \
    --filter=':- .gitignore' \
    --exclude '/build/' \
    --exclude '/build-asan/' \
    --exclude '/build-debug/' \
    --exclude '/build-local/' \
    --exclude '/ci_logs.zip' \
    --exclude '/clang-tidy-report.txt' \
    --exclude '/clang-tidy-report-full.txt' \
    "${ROOT_DIR}/" "${STAGE_DIR}/"
  if [[ -d "${STAGE_DIR}/.git" ]]; then
    (cd "${STAGE_DIR}" && git remote remove origin >/dev/null 2>&1 || true)
  fi
fi

if [[ ! -d "${STAGE_DIR}" ]]; then
  echo "Stage directory ${STAGE_DIR} is missing" >&2
  exit 1
fi

echo "==> Debug stage ready at ${STAGE_DIR}" >&2
INSTALL_SNIPPET=""
if [[ -n "${INSTALL_PKGS}" ]]; then
  INSTALL_SNIPPET="apt-get update >/dev/null && apt-get install -y --no-install-recommends ${INSTALL_PKGS} >/dev/null && "
fi

if [[ ${#USER_CMD[@]} -eq 0 ]]; then
  CONTAINER_ENTRY="${INSTALL_SNIPPET}exec bash"
else
  CONTAINER_ENTRY="${INSTALL_SNIPPET}exec ${USER_CMD[*]}"
fi

DOCKER_FLAGS=(--rm -e GITMIND_DOCKER=1 -v "${STAGE_DIR}:/workspace" -w /workspace)
if [[ -z "${INSTALL_PKGS}" ]]; then
  DOCKER_FLAGS+=(-u "$(id -u):$(id -g)")
fi
if [[ -t 1 ]]; then
  DOCKER_FLAGS+=( -it )
fi

echo "==> Launching ${IMAGE}" >&2
docker run "${DOCKER_FLAGS[@]}" "${IMAGE}" bash -lc "${CONTAINER_ENTRY}"
DOCKER_STATUS=$?

if [[ ${KEEP_STAGE} -ne 1 && ${AUTO_STAGE} -eq 1 ]]; then
  echo "==> Removing stage ${STAGE_DIR}" >&2
  rm -rf "${STAGE_DIR}"
elif [[ ${KEEP_STAGE} -ne 1 ]]; then
  echo "==> Stage left in place (user-provided): ${STAGE_DIR}" >&2
else
  echo "==> Keeping stage at ${STAGE_DIR}" >&2
fi

exit ${DOCKER_STATUS}
