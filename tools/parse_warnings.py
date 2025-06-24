#\!/usr/bin/env python3
import sys
import re
from collections import defaultdict

def parse_warnings(input_file):
    """Parse warnings and group by file + warning type."""
    warnings = defaultdict(int)
    
    # Pattern to match clang-tidy warnings
    pattern = r'^(.+?):(\d+):(\d+):\s*(warning|error):\s*(.+?)\s*\[(.+?)\]'
    
    with open(input_file, 'r') as f:
        for line in f:
            match = re.match(pattern, line.strip())
            if match:
                filepath = match.group(1)
                # Normalize path - remove ./ prefix
                filepath = filepath.replace('/Users/james/git/git-mind/core/./', '/Users/james/git/git-mind/core/')
                filepath = filepath.replace('./', '')
                
                warning_type = match.group(6)  # The [warning-name] part
                key = f"{filepath}:{warning_type}"
                warnings[key] += 1
    
    return warnings

def write_baseline(warnings, output_file):
    """Write the grouped baseline."""
    with open(output_file, 'w') as f:
        for key in sorted(warnings.keys()):
            count = warnings[key]
            f.write(f"{count}:{key}\n")

def compare_baselines(current_file, baseline_file):
    """Compare current warnings against baseline."""
    current = parse_warnings(current_file)
    
    # Parse baseline format (count:file:warning)
    baseline = {}
    with open(baseline_file, 'r') as f:
        for line in f:
            if ':' in line:
                parts = line.strip().split(':', 2)
                if len(parts) == 3:
                    count, filepath, warning = parts
                    key = f"{filepath}:{warning}"
                    baseline[key] = int(count)
    
    # Find new warnings
    new_warnings = []
    for key, count in current.items():
        baseline_count = baseline.get(key, 0)
        if count > baseline_count:
            new_warnings.append(f"+{count - baseline_count} new: {key}")
    
    return new_warnings

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: parse_warnings.py <command>")
        print("Commands:")
        print("  create-baseline <input> <output>")
        print("  check <current> <baseline>")
        sys.exit(1)
    
    command = sys.argv[1]
    
    if command == "create-baseline" and len(sys.argv) == 4:
        warnings = parse_warnings(sys.argv[2])
        write_baseline(warnings, sys.argv[3])
        print(f"Created baseline with {len(warnings)} unique warning types")
    
    elif command == "check" and len(sys.argv) == 4:
        new_warnings = compare_baselines(sys.argv[2], sys.argv[3])
        if new_warnings:
            print("❌ New warnings detected:")
            for w in new_warnings:
                print(f"  {w}")
            sys.exit(1)
        else:
            print("✅ No new warnings detected")
    
    else:
        print("Invalid command or arguments")
        sys.exit(1)
