#!/usr/bin/env bash
set -e
diff -u "$2" "$1" | (! grep '^+' | grep -v '^+++' )