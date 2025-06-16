#!/bin/bash
# SPDX-License-Identifier: Apache-2.0
# GitMind installation script

set -e

REPO="neuroglyph/neuroglyph"
INSTALL_DIR="/usr/local/bin"

# Detect OS and architecture
OS=$(uname -s | tr '[:upper:]' '[:lower:]')
ARCH=$(uname -m)

# Map architecture names
case "$ARCH" in
    x86_64) ARCH="x86_64" ;;
    amd64) ARCH="x86_64" ;;
    arm64) ARCH="arm64" ;;
    aarch64) ARCH="aarch64" ;;
    *) echo "Unsupported architecture: $ARCH"; exit 1 ;;
esac

# Determine binary name
case "$OS" in
    linux)
        if [ "$ARCH" = "arm64" ]; then
            ARCH="aarch64"
        fi
        BINARY="gitmind-linux-$ARCH"
        ;;
    darwin)
        # Use universal binary by default on macOS
        BINARY="gitmind-macos-universal"
        ;;
    mingw*|msys*|cygwin*|windows*)
        BINARY="gitmind-windows-x86_64.exe"
        INSTALL_DIR="$HOME/bin"
        ;;
    *)
        echo "Unsupported operating system: $OS"
        exit 1
        ;;
esac

echo "🔍 Detecting latest release..."
LATEST=$(curl -s "https://api.github.com/repos/$REPO/releases/latest" | grep '"tag_name":' | sed -E 's/.*"([^"]+)".*/\1/')

if [ -z "$LATEST" ]; then
    echo "❌ Failed to fetch latest release"
    exit 1
fi

echo "📦 Downloading GitMind $LATEST for $OS/$ARCH..."
DOWNLOAD_URL="https://github.com/$REPO/releases/download/$LATEST/$BINARY"

# Create temp directory
TMP_DIR=$(mktemp -d)
cd "$TMP_DIR"

# Download binary
if ! curl -fsSL -o gitmind "$DOWNLOAD_URL"; then
    echo "❌ Failed to download $BINARY"
    echo "URL: $DOWNLOAD_URL"
    rm -rf "$TMP_DIR"
    exit 1
fi

# Make executable
chmod +x gitmind

# Install
echo "📂 Installing to $INSTALL_DIR..."
if [ -w "$INSTALL_DIR" ]; then
    mv gitmind "$INSTALL_DIR/"
else
    echo "🔐 Administrator access required..."
    sudo mv gitmind "$INSTALL_DIR/"
fi

# Clean up
cd /
rm -rf "$TMP_DIR"

# Verify installation
if command -v gitmind >/dev/null 2>&1; then
    echo "✅ GitMind installed successfully!"
    echo ""
    gitmind version
else
    echo "⚠️  GitMind installed to $INSTALL_DIR but not in PATH"
    echo "Add $INSTALL_DIR to your PATH to use gitmind from anywhere"
fi