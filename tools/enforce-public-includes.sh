#!/usr/bin/env bash
set -euo pipefail

# Disallow including core/src implementation files from non-core code.
# Scan C/H files outside core/ for forbidden include patterns.

shopt -s nullglob

violations=()
while IFS= read -r -d '' file; do
  if grep -nE "#\s*include\s*\"(\.\./)?core/src/" "$file" >/dev/null; then
    lines=$(grep -nE "#\s*include\s*\"(\.\./)?core/src/" "$file")
    violations+=("$file:$lines")
  fi
done < <(git ls-files "*.c" "*.h" | grep -vE '^core/' | xargs -0 -I{} printf '%s\0' {})

if ((${#violations[@]})); then
  echo "Forbidden includes detected (must include public headers only):" >&2
  printf '%s\n' "${violations[@]}" >&2
  exit 1
fi

echo "Public-include enforcement passed."
