#!/bin/sh
# install.sh — Install @flyingrobots/git-mind globally
# Usage:
#   ./install.sh                          npm install -g
#   ./install.sh --prefix ~/.local        install to ~/.local/bin
#   curl -fsSL <raw-url> | sh             pipe-friendly
set -e

PACKAGE="@flyingrobots/git-mind"
MIN_NODE=22
PREFIX=""

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
    -h|--help)
      echo "Usage: $0 [--prefix <path>]"
      echo ""
      echo "Options:"
      echo "  --prefix <path>   Install to <path>/bin instead of global npm"
      echo ""
      echo "Examples:"
      echo "  $0                        # npm install -g"
      echo "  $0 --prefix ~/.local      # install to ~/.local/bin"
      exit 0
      ;;
    *)
      echo "Unknown option: $1" >&2
      exit 1
      ;;
  esac
done

# Check Node.js
if ! command -v node >/dev/null 2>&1; then
  echo "Error: Node.js is not installed." >&2
  echo "Install Node.js >= ${MIN_NODE} from https://nodejs.org" >&2
  exit 1
fi

NODE_MAJOR=$(node -e "process.stdout.write(String(process.versions.node.split('.')[0]))")
if [ "$NODE_MAJOR" -lt "$MIN_NODE" ]; then
  echo "Error: Node.js >= ${MIN_NODE} required (found v$(node -v))" >&2
  exit 1
fi

# Check npm
if ! command -v npm >/dev/null 2>&1; then
  echo "Error: npm is not installed." >&2
  exit 1
fi

echo "Installing ${PACKAGE}..."

if [ -n "$PREFIX" ]; then
  # Expand ~ if present
  PREFIX=$(eval echo "$PREFIX")
  npm install -g --prefix "$PREFIX" "$PACKAGE"

  BIN_DIR="${PREFIX}/bin"
  echo ""
  echo "Installed to ${BIN_DIR}/git-mind"
  echo ""

  # Check if BIN_DIR is in PATH
  case ":$PATH:" in
    *":${BIN_DIR}:"*) ;;
    *)
      echo "Add ${BIN_DIR} to your PATH:"
      echo ""
      echo "  export PATH=\"${BIN_DIR}:\$PATH\""
      echo ""
      echo "Add that line to your ~/.bashrc, ~/.zshrc, or ~/.profile"
      ;;
  esac
else
  npm install -g "$PACKAGE"
fi

echo ""
echo "Done! Run 'git mind --version' to verify."
