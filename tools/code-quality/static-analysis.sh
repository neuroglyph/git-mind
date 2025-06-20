#!/bin/bash
# SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0
# © 2025 J. Kirby Ross / Neuroglyph Collective

# Run comprehensive static analysis

set -e

echo "=== Git-Mind Static Analysis Suite ==="
echo

FAILED=0
REPORT_DIR="static-analysis-reports"
mkdir -p "$REPORT_DIR"

# 1. Clang Static Analyzer (scan-build)
if command -v scan-build &> /dev/null; then
    echo ">>> Running Clang Static Analyzer..."
    scan-build -o "$REPORT_DIR/scan-build" \
               --use-cc=gcc \
               -enable-checker security.insecureAPI \
               -enable-checker security.FloatLoopCounter \
               -enable-checker alpha.security.ArrayBoundV2 \
               make -C src clean all 2>&1 | tee "$REPORT_DIR/scan-build.log"
    
    if [ -d "$REPORT_DIR/scan-build/"* ]; then
        echo "⚠️  Static analyzer found issues. See $REPORT_DIR/scan-build/"
        FAILED=$((FAILED + 1))
    else
        echo "✅ No issues found by scan-build"
    fi
    echo
fi

# 2. PVS-Studio (if available)
if command -v pvs-studio-analyzer &> /dev/null; then
    echo ">>> Running PVS-Studio..."
    pvs-studio-analyzer analyze -o "$REPORT_DIR/pvs.log" \
                               -e /usr/include \
                               -j8
    plog-converter -a GA:1,2 -t errorfile "$REPORT_DIR/pvs.log" -o "$REPORT_DIR/pvs-report.txt"
    
    if [ -s "$REPORT_DIR/pvs-report.txt" ]; then
        echo "⚠️  PVS-Studio found issues:"
        cat "$REPORT_DIR/pvs-report.txt"
        FAILED=$((FAILED + 1))
    else
        echo "✅ PVS-Studio passed"
    fi
    echo
fi

# 3. Coverity (if available)
if command -v cov-build &> /dev/null; then
    echo ">>> Running Coverity..."
    cov-build --dir "$REPORT_DIR/coverity" make -C src clean all
    cov-analyze --dir "$REPORT_DIR/coverity" \
                --all \
                --enable-constraint-fpp \
                --enable-fnptr \
                --enable-virtual
    cov-format-errors --dir "$REPORT_DIR/coverity" \
                     --html-output "$REPORT_DIR/coverity-html"
    echo "✅ Coverity report generated in $REPORT_DIR/coverity-html"
    echo
fi

# 4. GCC Static Analysis (-fanalyzer)
if gcc --version | grep -q "gcc-1[0-9]"; then
    echo ">>> Running GCC Analyzer..."
    make -C src clean
    if make -C src CFLAGS="-fanalyzer -Wanalyzer-too-complex" 2>&1 | \
       tee "$REPORT_DIR/gcc-analyzer.log" | \
       grep -E "(warning|error).*analyzer"; then
        echo "⚠️  GCC analyzer found issues"
        FAILED=$((FAILED + 1))
    else
        echo "✅ GCC analyzer passed"
    fi
    echo
fi

# 5. Infer (Facebook's analyzer)
if command -v infer &> /dev/null; then
    echo ">>> Running Infer..."
    infer run --reactive -- make -C src
    if [ -s "infer-out/report.txt" ]; then
        cp infer-out/report.txt "$REPORT_DIR/infer-report.txt"
        echo "⚠️  Infer found issues:"
        cat "$REPORT_DIR/infer-report.txt"
        FAILED=$((FAILED + 1))
    else
        echo "✅ Infer passed"
    fi
    echo
fi

# 6. CodeQL (GitHub's analyzer)
if command -v codeql &> /dev/null; then
    echo ">>> Running CodeQL..."
    codeql database create "$REPORT_DIR/codeql-db" --language=cpp --command="make -C src"
    codeql database analyze "$REPORT_DIR/codeql-db" \
           --format=csv \
           --output="$REPORT_DIR/codeql-results.csv" \
           cpp-queries:codeql-suites/cpp-security-and-quality.qls
    
    if [ -s "$REPORT_DIR/codeql-results.csv" ]; then
        echo "⚠️  CodeQL found issues. See $REPORT_DIR/codeql-results.csv"
        FAILED=$((FAILED + 1))
    else
        echo "✅ CodeQL passed"
    fi
    echo
fi

# 7. Flawfinder (security-focused)
if command -v flawfinder &> /dev/null; then
    echo ">>> Running Flawfinder..."
    flawfinder --minlevel=2 \
               --html \
               --context \
               src/ > "$REPORT_DIR/flawfinder.html" 2>&1
    
    if flawfinder --quiet --minlevel=2 src/ 2>&1 | grep -q "ANALYSIS SUMMARY"; then
        echo "⚠️  Flawfinder found security issues. See $REPORT_DIR/flawfinder.html"
        FAILED=$((FAILED + 1))
    else
        echo "✅ Flawfinder passed"
    fi
    echo
fi

# Summary
echo "=== Static Analysis Summary ==="
echo "Reports saved in: $REPORT_DIR/"
if [ $FAILED -eq 0 ]; then
    echo "✅ All static analyzers passed!"
else
    echo "❌ $FAILED analyzers found issues"
    echo "Review reports in $REPORT_DIR/"
    exit 1
fi