# Syscall research

Public headers describe caller-facing contracts. These directories hold the supporting
investigation: ROM observations, test methods, hypotheses, and
remaining questions. They are not a second maintained API reference.

- [Availability by ROM version](rom-availability.md)
- [System selectors and dispatch](system/README.md)
- [Applet lifecycle and messages](applet/README.md)
- [Dialogs and text input](dialog/README.md)
- [Display and graphics](display/README.md)
- [Files and clipboard](file/README.md)
- [Keyboard and key translation](keyboard/README.md)
- [Localization and character repertoire](localization/README.md)
- [Power, battery, and shared dialog probes](power/README.md)
- [Restricted markup scanner, slots 271–276](markup/README.md)
- [Speech queue and controls, slots 282–283](speech/README.md)
- [Wireless packets, association, transfers, and receive status](wireless/README.md)
- [Serial ports and callbacks](serial/README.md)
- [IrDA and slots 146–173 integration](irda/README.md)
- [Native text editor, scanning, statistics, and forms](text/README.md)
- [Standard-library services and wrappers](stdlib/README.md)
- [Timers, uptime, and busy loops](time/README.md)

## Evidence and confidence

Each subsystem describes its actual coverage. A disassembly observation establishes
what the inspected firmware does; a MAME probe establishes only the exercised path
and emulated hardware behavior. Neither alone proves every device, ROM, or input.
Hypotheses and unresolved fields remain explicitly identified. Manufacturer applet
callers supply evidence of usage, not permission to infer an untested contract.

Each topic has a README containing findings, disassembly evidence, confidence,
and unresolved questions. Maintained C/Python/Lua test sources live beside the README.
Topics without maintained probes have only a README and state their coverage gaps.
Obsolete exploratory scripts and reproducible execution logs are not retained.
No firmware binaries are copied here. The localization comparison bitmap is kept
as a useful visual artifact requested during the investigation.

## Reproducing maintained checks

From the workspace root:

```sh
python3 betawise/research/abi-audit/run.py /tmp/os3k-syscall-validation
```

The shared [ABI audit](../abi-audit/README.md),
[ROM inventory](../abi-audit/rom-inventory.json), and
[vendor-image inventory](../abi-audit/vendor-image-inventory.json) identify inputs.
The runner first checks all eighteen production pointer bridges under QEMU,
then checks compiler conventions, raw pointer behavior, variadic and aggregate
cases, serial callbacks, localization/idle settings, and battery/dialog cases on
AS3000 3.01 and Neo 3.21. It does not exhaustively certify every documented syscall.
Neo 3.01 observations must be distinguished from live Neo 3.21 tests.

Maintained subsystem probes live beside their topic README.
Mixed battery/dialog tests live once under `power/` and are linked from
other topics. Shared ABI probes stay under `abi-audit/`. The runner generates
logs and binaries in its supplied output directory; those outputs are disposable.
Static disassembly records and important observations stay in the topic README.

## Organization checks (2026-11-18)

The initial header-to-research move preserved all C tokens and passed Doxygen
with warnings treated as errors, the library build, and all eleven integration
groups. The simplified flat layout was checked again with the same eleven-group
runner, the localization inventory scan, Python compilation, and local Markdown
link checks. Generated outputs stay in temporary directories.


Probe filenames describe their purpose; the shared runner handles execution order.
