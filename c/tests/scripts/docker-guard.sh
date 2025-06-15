#!/bin/bash
# SPDX-License-Identifier: Apache-2.0
# Docker Guard - Prevents test execution outside Docker

# Function to check if we're in Docker
check_docker() {
    # Method 1: Check for .dockerenv file
    if [ -f /.dockerenv ]; then
        return 0
    fi
    
    # Method 2: Check cgroup for docker
    if [ -f /proc/1/cgroup ] && grep -q docker /proc/1/cgroup 2>/dev/null; then
        return 0
    fi
    
    # Method 3: Check for DOCKER_CONTAINER env var (escape hatch)
    if [ "${DOCKER_CONTAINER:-}" = "1" ]; then
        echo "⚠️  WARNING: DOCKER_CONTAINER=1 override detected"
        return 0
    fi
    
    # Method 4: Check if running in GitHub Actions (for cross-platform testing)
    if [ "${GITHUB_ACTIONS:-}" = "true" ]; then
        echo "⚠️  Running in GitHub Actions - Docker check bypassed for cross-platform testing"
        return 0
    fi
    
    # Not in Docker!
    return 1
}

# Perform the check
if ! check_docker; then
    echo "❌ FATAL ERROR: This script must be run inside Docker!"
    echo ""
    echo "DO NOT run tests directly. Instead use:"
    echo "  From root:  make test"
    echo "  From c/:    docker compose run --rm test"
    echo ""
    echo "This restriction exists because:"
    echo "  1. Tests create real Git repositories"
    echo "  2. Tests perform real Git operations"
    echo "  3. Running tests on your working repo is DANGEROUS"
    echo ""
    echo "If you REALLY know what you're doing (you don't), you can bypass with:"
    echo "  DOCKER_CONTAINER=1 $0"
    echo ""
    exit 1
fi

echo "✅ Running inside Docker container"