#!/bin/sh
# uninstall.sh — Remove @flyingrobots/git-mind
# Usage:
#   ./uninstall.sh                          npm uninstall -g
#   ./uninstall.sh --prefix ~/.local        remove from ~/.local
set -e

PACKAGE="@flyingrobots/git-mind"
PREFIX=""
CLEAN_CACHE=""

# Parse arguments
while [ $# -gt 0 ]; do
  case "$1" in
    --prefix)
      PREFIX="$2"
      shift 2
      ;;
    --prefix=*)
      PREFIX="${1#*=}"
      shift
      ;;
    --clean-cache)
      CLEAN_CACHE=1
      shift
      ;;
    -h|--help)
      echo "Usage: $0 [--prefix <path>] [--clean-cache]"
      echo ""
      echo "Options:"
      echo "  --prefix <path>   Uninstall from <path> instead of global npm"
      echo "  --clean-cache     Also remove ~/.gitmind/ cache directory"
      exit 0
      ;;
    *)
      echo "Unknown option: $1" >&2
      exit 1
      ;;
  esac
done

echo "Removing ${PACKAGE}..."

if [ -n "$PREFIX" ]; then
  PREFIX=$(eval echo "$PREFIX")
  npm uninstall -g --prefix "$PREFIX" "$PACKAGE"
else
  npm uninstall -g "$PACKAGE"
fi

echo "Package removed."

# Optionally clean cache
CACHE_DIR="${HOME}/.gitmind"
if [ -n "$CLEAN_CACHE" ] && [ -d "$CACHE_DIR" ]; then
  rm -rf "$CACHE_DIR"
  echo "Cache directory ${CACHE_DIR} removed."
elif [ -z "$CLEAN_CACHE" ] && [ -d "$CACHE_DIR" ]; then
  echo ""
  echo "Note: Cache directory ${CACHE_DIR} still exists."
  echo "Run with --clean-cache to remove it."
fi

echo ""
echo "Done!"
