#!/usr/bin/env bash
set -euo pipefail

# This runs INSIDE the Docker container and orchestrates parallel checks

echo "🚀 Orchestrating parallel CI checks..."

# Function to run a check in background
run_check() {
    local name=$1
    local cmd=$2
    local logfile="/tmp/check-$name.log"
    
    echo "→ Starting $name..."
    bash -c "$cmd" >$logfile 2>&1 &
    echo "$! $name $logfile"
}

# Function to wait for all checks and report results
wait_for_checks() {
    local failed=0
    local pids=("$@")
    
    for pid_info in "${pids[@]}"; do
        local pid=$(echo $pid_info | cut -d' ' -f1)
        local name=$(echo $pid_info | cut -d' ' -f2)
        local logfile=$(echo $pid_info | cut -d' ' -f3)
        
        if wait $pid; then
            echo "✅ $name passed"
        else
            echo "❌ $name FAILED"
            echo "--- Output from $name ---"
            cat $logfile
            echo "--- End of $name output ---"
            failed=1
        fi
    done
    
    return $failed
}

# Quick checks first
echo ""
echo "═══ Quick Checks ═══"
echo "→ Checking for TODOs/FIXMEs..."
if grep -nE "(TODO|FIXME|XXX)" $(find core -name "*.c" -o -name "*.h" 2>/dev/null); then
    echo "❌ NO TODOs ALLOWED"
    exit 1
fi
echo "✅ No TODOs found"

echo "→ Basic build check..."
CC=clang meson setup build-quick >/dev/null 2>&1
ninja -C build-quick >/dev/null 2>&1
echo "✅ Build successful"

# Start all checks in parallel
echo ""
echo "═══ Running Full CI Suite in Parallel ═══"

pids=()

# clang-tidy
pids+=("$(run_check "clang-tidy" '
    set -e
    CC=clang meson setup build >/dev/null 2>&1
    ninja -C build >/dev/null 2>&1
    cp build/compile_commands.json .
    clang-tidy -quiet -p . --config-file=quality/.clang-tidy \
        $(git ls-files "core/**/*.c" "core/**/*.h") \
        | tee clang-tidy-report-full.txt || true
    grep -E "^/.*core/|^core/" clang-tidy-report-full.txt > clang-tidy-report.txt || true
    python3 tools/count_warnings.py check clang-tidy-report.txt tools/baseline_count.txt
')")

# cppcheck
pids+=("$(run_check "cppcheck" '
    cppcheck --enable=all --inconclusive --quiet src 2>&1
')")

# ASAN
pids+=("$(run_check "ASAN" '
    set -e
    CC=clang CFLAGS="-fsanitize=address -fno-omit-frame-pointer -g" \
        meson setup build-asan -Db_sanitize=address >/dev/null 2>&1
    ninja -C build-asan >/dev/null 2>&1
    ASAN_OPTIONS=detect_leaks=1:strict_string_checks=1:detect_stack_use_after_return=1:check_initialization_order=1:strict_init_order=1 \
        ninja -C build-asan test
')")

# UBSAN
pids+=("$(run_check "UBSAN" '
    set -e
    CC=clang CFLAGS="-fsanitize=undefined -fno-omit-frame-pointer -g" \
        meson setup build-ubsan -Db_sanitize=undefined >/dev/null 2>&1
    ninja -C build-ubsan >/dev/null 2>&1
    UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1 \
        ninja -C build-ubsan test
')")

# Standard tests
pids+=("$(run_check "Standard-Tests" '
    set -e
    CC=clang meson setup build-standard >/dev/null 2>&1
    ninja -C build-standard >/dev/null 2>&1
    ninja -C build-standard test
')")

# Coverage
pids+=("$(run_check "Coverage" '
    set -e
    CC=clang meson setup build-coverage -Db_coverage=true >/dev/null 2>&1
    ninja -C build-coverage test >/dev/null 2>&1
    cd build-coverage
    gcovr --root .. --print-summary | grep -E "lines|branches"
    gcovr --root .. --fail-under-line 80 --fail-under-branch 70 >/dev/null || {
        echo "❌ Coverage requirements not met! (80% line, 70% branch required)"
        exit 1
    }
')")

# Fuzzing
pids+=("$(run_check "Fuzzing" '
    python tools/run_fuzz.py 60
')")

# Wait for all parallel checks
echo ""
echo "⏳ Waiting for all checks to complete..."
echo ""

if wait_for_checks "${pids[@]}"; then
    echo ""
    echo "═══════════════════════════════════"
    echo "✅ ALL CI CHECKS PASSED!"
    echo "═══════════════════════════════════"
    exit 0
else
    echo ""
    echo "═══════════════════════════════════"
    echo "❌ SOME CHECKS FAILED!"
    echo "═══════════════════════════════════"
    exit 1
fi