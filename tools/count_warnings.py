#!/usr/bin/env python3
import sys

def count_warnings(input_file):
    """Count total warnings in clang-tidy output."""
    count = 0
    with open(input_file, 'r') as f:
        for line in f:
            if ': warning:' in line or ': error:' in line:
                count += 1
    return count

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: count_warnings.py <command>")
        print("Commands:")
        print("  create-baseline <input> <output>")
        print("  check <current> <baseline>")
        sys.exit(1)
    
    command = sys.argv[1]
    
    if command == "create-baseline" and len(sys.argv) == 4:
        count = count_warnings(sys.argv[2])
        with open(sys.argv[3], 'w') as f:
            f.write(f"{count}\n")
        print(f"✅ Created baseline: {count} warnings")
    
    elif command == "check" and len(sys.argv) == 4:
        current_count = count_warnings(sys.argv[2])
        with open(sys.argv[3], 'r') as f:
            baseline_count = int(f.read().strip())
        
        if current_count > baseline_count:
            print(f"❌ New warnings detected: {current_count} (baseline: {baseline_count})")
            sys.exit(1)
        else:
            print(f"✅ No new warnings: {current_count} (baseline: {baseline_count})")
    
    else:
        print("Invalid command or arguments")
        sys.exit(1)