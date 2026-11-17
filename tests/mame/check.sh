#!/usr/bin/env bash
set -euo pipefail

if (( $# < 2 || $# > 3 )); then
    echo "usage: $0 MAME ROM_DIRECTORY [hello|memory|invoke|syscall|string|handler]" >&2
    exit 2
fi
mame=$(realpath "$1")
roms=$(realpath "$2")
test_dir=$(cd -- "$(dirname -- "$0")" && pwd)
root=$(cd -- "$test_dir/../.." && pwd)
nm=${CROSS_COMPILE:-m68k-linux-gnu-}nm
if (( $# == 3 )); then
    cases=("$3")
else
    cases=(hello memory invoke syscall string handler)
fi

work=$(mktemp -d)
echo "MAME test output: $work"
for test_case in "${cases[@]}"; do
    case "$test_case" in
        hello) applet=HelloWorld; seconds=12 ;;
        memory) applet=DebugTool; seconds=57 ;;
        invoke) applet=DebugTool; seconds=65 ;;
        syscall) applet=DebugTool; seconds=26 ;;
        string) applet=DebugTool; seconds=46 ;;
        handler) applet=DebugTool; seconds=30 ;;
        *) echo "Unknown case: $test_case" >&2; exit 2 ;;
    esac

    artifact="$root/applets/$applet/$applet"
    if [[ ! -f $artifact.OS3KApp || ! -f $artifact.elf ]]; then
        echo "Build the applets with make before running this check." >&2
        exit 2
    fi

    run="$work/$test_case"
    mkdir "$run"
    {
        printf 'local test_case = "%s"\nlocal symbols = {\n' "$test_case"
        "$nm" -P "$artifact.elf" |
            awk '$1 ~ /^(g_scratch|g_pAddress|g_cursor|g_busError|BusErrorHandler|__header)$/ {
                printf "    %s = 0x%s,\n", $1, $3
            }'
        printf '}\n'
        cat "$test_dir/asma3k.lua"
    } > "$run/check.lua"

    (
        cd "$run"
        "$mame" asma3k -rompath "$roms" -nvram_directory nvram \
            -snapshot_directory snap -applet "$artifact.OS3KApp" \
            -video none -sound none -nothrottle -seconds_to_run "$seconds" \
            -autoboot_delay 0 -autoboot_script "$run/check.lua" > mame.log 2>&1
    )
    if ! grep -Fxq "PASS $test_case" "$run/mame.log" || grep -q 'LUA ERROR' "$run/mame.log"; then
        tail -30 "$run/mame.log" >&2
        echo "MAME check failed: $test_case ($run)" >&2
        exit 1
    fi
    echo "PASS $test_case"
done
