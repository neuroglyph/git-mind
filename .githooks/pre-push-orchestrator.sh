#!/usr/bin/env bash
set -euo pipefail

# This runs INSIDE the Docker container and orchestrates parallel checks

echo "🚀 Orchestrating parallel CI checks..."

# Track background jobs
declare -A job_names
declare -A job_logs

# Function to run a check in background
run_check() {
    local name=$1
    local cmd=$2
    local logfile="/tmp/check-$name.log"
    
    echo "→ Starting $name..."
    bash -c "$cmd" >$logfile 2>&1 &
    local pid=$!
    job_names[$pid]="$name"
    job_logs[$pid]="$logfile"
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
if [ ! -d build-quick ]; then
    CC=clang meson setup build-quick >/dev/null 2>&1
fi
ninja -C build-quick >/dev/null 2>&1
echo "✅ Build successful"

# Start all checks in parallel
echo ""
echo "═══ Running Full CI Suite in Parallel ═══"

# clang-tidy
run_check "clang-tidy" '
    set -e
    if [ ! -d build ]; then
        CC=clang meson setup build >/dev/null 2>&1
    fi
    ninja -C build >/dev/null 2>&1
    cp build/compile_commands.json .
    clang-tidy -quiet -p . --config-file=quality/.clang-tidy \
        $(git ls-files "core/**/*.c" "core/**/*.h") \
        | tee clang-tidy-report-full.txt || true
    grep -E "^/.*core/|^core/" clang-tidy-report-full.txt > clang-tidy-report.txt || true
    python3 tools/count_warnings.py check clang-tidy-report.txt tools/baseline_count.txt
'

# cppcheck
run_check "cppcheck" '
    cppcheck --enable=all --inconclusive --quiet src 2>&1
'

# ASAN
run_check "ASAN" '
    set -e
    if [ ! -d build-asan ]; then
        CC=clang CFLAGS="-fsanitize=address -fno-omit-frame-pointer -g" \
            meson setup build-asan -Db_sanitize=address >/dev/null 2>&1
    fi
    ninja -C build-asan >/dev/null 2>&1
    ASAN_OPTIONS=detect_leaks=1:strict_string_checks=1:detect_stack_use_after_return=1:check_initialization_order=1:strict_init_order=1 \
        ninja -C build-asan test
'

# UBSAN
run_check "UBSAN" '
    set -e
    if [ ! -d build-ubsan ]; then
        CC=clang CFLAGS="-fsanitize=undefined -fno-omit-frame-pointer -g" \
            meson setup build-ubsan -Db_sanitize=undefined >/dev/null 2>&1
    fi
    ninja -C build-ubsan >/dev/null 2>&1
    UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1 \
        ninja -C build-ubsan test
'

# Standard tests
run_check "Standard-Tests" '
    set -e
    if [ ! -d build-standard ]; then
        CC=clang meson setup build-standard >/dev/null 2>&1
    fi
    ninja -C build-standard >/dev/null 2>&1
    ninja -C build-standard test
'

# Coverage
run_check "Coverage" '
    set -e
    if [ ! -d build-coverage ]; then
        CC=clang meson setup build-coverage -Db_coverage=true >/dev/null 2>&1
    fi
    ninja -C build-coverage test >/dev/null 2>&1
    cd build-coverage
    gcovr --root .. --print-summary | grep -E "lines|branches"
    gcovr --root .. --fail-under-line 80 --fail-under-branch 70 >/dev/null || {
        echo "❌ Coverage requirements not met! (80% line, 70% branch required)"
        exit 1
    }
'

# Fuzzing
run_check "Fuzzing" '
    python3 tools/run_fuzz.py 60
'

# Wait for all parallel checks
echo ""
echo "⏳ Waiting for all checks to complete..."
echo "💡 Tip: tail -f /tmp/check-*.log to watch progress"
echo ""

# Wait for all background jobs and collect results
failed=0
for pid in $(jobs -p); do
    if wait $pid; then
        echo "✅ ${job_names[$pid]} passed"
    else
        echo "❌ ${job_names[$pid]} FAILED"
        echo "--- Output from ${job_names[$pid]} ---"
        if [ -s "${job_logs[$pid]}" ]; then
            cat "${job_logs[$pid]}"
        else
            echo "(no output captured - check /tmp/check-${job_names[$pid]}.log)"
        fi
        echo "--- End of ${job_names[$pid]} output ---"
        failed=1
    fi
done

if [ $failed -eq 0 ]; then
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