#!/usr/bin/env bash
set -euo pipefail
root=$(cd -- "$(dirname -- "$0")/../.." && pwd)
work=$(mktemp -d /tmp/betawise-console.XXXXXX)
cc -std=c99 -fno-builtin -Wall -Wextra -I"$root/os3k" \
    "$root/tests/console/console.c" "$root/os3k/os3k_console.c" "$root/os3k/os3k_stdlib.c" "$root/os3k/os3k_console_input.c" -o "$work/console"
"$work/console"
echo "PASS console parser and character streams"
