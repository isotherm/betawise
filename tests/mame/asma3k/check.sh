#!/usr/bin/env bash
set -euo pipefail

if (( $# > 2 )); then
    echo "usage: $0 [MAME [hello|memory|invoke|syscall|string|handler|micropython|micropython-minimal|micropython-core|micropython-basic|micropython-extra]]" >&2
    exit 2
fi
test_dir=$(cd -- "$(dirname -- "$0")" && pwd)
root=$(cd -- "$test_dir/../../.." && pwd)
mame=$(realpath "${1:-$root/../mame/mame}")
nm=${CROSS_COMPILE:-m68k-linux-gnu-}nm
if (( $# == 2 )); then
    cases=("$2")
else
    cases=(hello memory invoke syscall string handler micropython-core)
fi

mame_roms="$(dirname -- "$mame")/roms"
full_rom="$test_dir/OS3KROM.OS3KOS"
small_rom="$test_dir/SMALLOS3KROM.OS3KOS"
if [[ ! -f $small_rom ]]; then
    small_rom="$mame_roms/asma3k/smallos3krom.os3kos"
fi
if [[ ! -f $full_rom || ! -f $small_rom ]]; then
    echo "Place OS3KROM.OS3KOS and SMALLOS3KROM.OS3KOS beside check.sh (see tests/mame/asma3k/README.md)." >&2
    exit 2
fi

work=$(mktemp -d)
roms="$work/roms"
mkdir -p "$roms/asma3k"
ln -s "$full_rom" "$roms/asma3k/os3krom.os3kos"
ln -s "$small_rom" "$roms/asma3k/smallos3krom.os3kos"
echo "MAME test output: $work"
failures=0
for test_case in "${cases[@]}"; do
    variant=
    case "$test_case" in
        hello) applet=HelloWorld; seconds=12 ;;
        memory) applet=DebugTool; seconds=57 ;;
        invoke) applet=DebugTool; seconds=65 ;;
        syscall) applet=DebugTool; seconds=26 ;;
        string) applet=DebugTool; seconds=46 ;;
        handler) applet=DebugTool; seconds=30 ;;
        micropython) applet=MicroPython; seconds=900 ;;
        micropython-facilities-extra)
            applet=MicroPython; seconds=420; variant=extra ;;
        micropython-repl-minimal|micropython-repl-core|micropython-repl-basic|micropython-repl-extra)
            applet=MicroPython; seconds=480; variant=${test_case#micropython-repl-} ;;
        micropython-minimal|micropython-core|micropython-basic|micropython-extra)
            applet=MicroPython; seconds=600; variant=${test_case#micropython-}
            if [[ $variant == extra ]]; then seconds=1200; fi ;;
        *) echo "Unknown case: $test_case" >&2; exit 2 ;;
    esac

    artifact="$root/applets/$applet/$applet"
    if [[ $applet == MicroPython ]]; then
        artifact="$root/applets/MicroPython/MicroPython-core"
    fi
    script="$applet.lua"
    if [[ -n $variant ]]; then
        artifact="$root/applets/MicroPython/build-$variant/MicroPython"
        script=MicroPythonProfile.lua
        if [[ $test_case == micropython-repl-* ]]; then script=MicroPythonRepl.lua; fi
        if [[ $test_case == micropython-facilities-* ]]; then script=MicroPythonFacilities.lua; fi
    fi
    if [[ ! -f $artifact.OS3KApp || ( $applet == DebugTool && ! -f $artifact.elf ) ]]; then
        echo "Build $applet before running this check (see tests/mame/asma3k/README.md)." >&2
        exit 2
    fi

    run="$work/$test_case"
    mkdir "$run"
    {
        printf 'local test_case = "%s"\n' "$test_case"
        printf 'local variant = "%s"\n' "$variant"
        if [[ $applet == DebugTool ]]; then
            printf 'local symbols = {\n'
            "$nm" -P "$artifact.elf" |
                awk '$1 ~ /^(g_scratch|g_pAddress|g_cursor|g_busError|BusErrorHandler|__header)$/ {
                    printf "    %s = 0x%s,\n", $1, $3
                }'
            printf '}\n'
        fi
        cat "$test_dir/asma3k.lua" "$test_dir/$script"
    } > "$run/check.lua"

    echo "RUN  $test_case"
    if (
        cd "$run"
        SDL_VIDEODRIVER=dummy "$mame" asma3k -rompath "$roms;$mame_roms" -nvram_directory nvram \
            -snapshot_directory snap -applet "$artifact.OS3KApp" \
            -video none -sound none -nothrottle -seconds_to_run "$seconds" \
            -autoboot_delay 0 -autoboot_script "$run/check.lua" > mame.log 2>&1
    ) && grep -Fxq "PASS $test_case" "$run/mame.log" && ! grep -q 'LUA ERROR' "$run/mame.log"; then
        echo "PASS $test_case"
    else
        tail -30 "$run/mame.log" >&2
        echo "FAIL $test_case ($run/mame.log)" >&2
        failures=$((failures + 1))
    fi
done
echo "$((${#cases[@]} - failures))/${#cases[@]} MAME checks passed"
(( failures == 0 ))
