# Complete syscall execution checks

Run from the workspace root with the private original ROM images and existing
`mame/mame` (AS3000) and `mame/neodiag` (Neo) builds available:

```sh
python3 betawise/research/syscalls/coverage/run.py /path/on/disk/os3k-validation
```

Use a fresh output directory. The runner fails on a failed suite, mismatched ROM,
unaccounted dispatch slot, missing public API documentation, or defined syscall
without observed dispatch evidence. It runs one MAME instance at a time. Generated
binaries, NVRAM, snapshots, reports, and documentation are not repository inputs.

## Coverage rules

There are 264 defined syscalls and 21 empty slots in the current source. QEMU
checks all eighteen production pointer-return adapters, including argument
forwarding and NULL/non-NULL D0 results. MAME checks firmware behavior using the
original AS3000 3.01 and Neo 3.21 ROMs. The Neo 3.01 image is hash/table checked;
it is not live-tested by this runner. No claim is made about other ROM releases.

A read-only pass-through tap records native dispatch-table loads after each
fixture takes control. The generated `coverage.tsv` maps every defined syscall
to passing MAME logs; `summary.json` records the totals and ROM scope. No
evidence from a failed suite counts. Each populated entry must be exercised on
both live ROMs where present (219 AS3000 entries and 264 Neo entries). Table
reads establish dispatch, not exhaustive branch coverage. Behavioral assertions
in each suite establish the tested result, side effect, error, or native UI
exit. Indirect A-line calls within tested workflows can also appear in the
report. The report distinguishes execution coverage from QEMU's mocked adapter
checks; mock success alone cannot qualify a ROM routine.

Empty entries and calls outside an older ROM's table are checked statically and
never invoked. Table hashes, sizes, and targets are checked against the original
[ROM inventory](../../abi-audit/rom-inventory.json).

## Maintained groups

| Group | Checks |
|---|---|
| Core display | Cursor and text output, AS LCD DDRAM, shadow writes, snapshot restoration, progress, Neo bitmap pixels and all seven raster operations. |
| Core keyboard/dialog | Queue order, empty/readiness behavior, modifiers, matrix sampling, accessibility getters/setters, fresh key waits, input exit, list selection and IDs. |
| Core time | BusyLoop, independent counters, centisecond wait, timer range rejection, cancellation, stale handles, and native interrupt-driven expiry flags/callbacks. |
| Core files | Seek/read/insert/delete, length/save/restore/reservation, current file and folder, flags, password read/write and cancel paths, real create/delete, tags, clipboard copy/cut/paste/read/resize/clear. |
| Core applets | Identity selection/clear, rejected selection, name/ID lookup, message callback parameters/reply, printer UI cancellation, unsupported system selector. |
| Core stdlib | Early string/formatter helpers, errno, input and echo, scalar/variadic stream calls, decimal parsing, comparisons, random seed, byte conversion, ungetc, and abort's reset-SP/A6 firmware transfer. |
| ABI | Original pointer conventions, variadic/aggregate conventions, compiled adapters, serial callbacks, localization/power, battery and dialog cases. |
| IrDA | Services, channel validation, callbacks, masks/resources, password/user UI, event dispatch and exception record. Controlled transport boundaries are documented in that topic. |
| Text | Editor, scanner, statistics, forms and transfer entry points. |
| Neo dialogs | Font-aware objects, shared input state, properties, animations, text width/wrapping, bitmap registration, compiled constructor adapter. |
| Markup | Six aggregate-return adapters, parser/filter, attributes/text, relative child search and truncation. |
| Wireless/speech | Packet/state/transfer and callback fixtures; speech state/queue. Physical peers and audible output are not certified. |

Core groups can be run separately:

```sh
python3 betawise/research/syscalls/coverage/run-core.py /path/on/disk/core \
  --group time --rom neo_3_21
python3 betawise/research/syscalls/coverage/check-docs.py
```

## Fixtures and limits

Core fixtures seed disposable RAM file records and applet descriptors, or pulse
native keyboard fields for modal UI. Fresh-event queue injection executes the
original QueueKey syscall with registers/flags preserved. Timer expiry uses the
normal interrupt path, including a callback that records its inherited A5 and
notification event; a cancelled callback stays silent past its deadline. The
abort check observes the original firmware entry with its expected reset stack,
zero A6 and abort flag; merely failing to return is not accepted. Neither ROM
syscall bodies nor their result bytes are patched by the core fixture or
dispatch observer.

For allocation tests, assertions read live record addresses: free-space and
clipboard operations can relocate buffers. The applet-ID fixture includes a
system descriptor because native search stops at the first missing descriptor.
The fixtures do not establish native installation, flash persistence, allocator
stress, all password-reset success paths, every template/navigation branch,
callback A5 safety after focus changes, or physical transport/electrical timing.
See the topic READMEs for those limits.

Doxygen checks every public syscall's brief, parameters, and non-void return
(including retval alternatives). HTML and XML are generated only in a temporary
directory and deleted. Doxygen 1.9.4 produces false missing-parameter warnings
when HTML generation is disabled, so the check generates and discards HTML too.

## Verified run (2026-11-19)

The complete runner passes: 219/219 populated AS3000 3.01 entries, 264/264 Neo
3.21 entries, and all 21 declared empty slots. The three original ROM tables
match their hashes/inventories. QEMU startup/relocation checks and eighteen
production pointer-bridge checks pass; Doxygen syntax and parameter/return
coverage pass for all 264 public syscall APIs. Additional native held/released
L-key checks pass on both machines. Generated coverage reports remain in the
chosen output directory and are recreated by the runner.

The observer's start frame matters: compiled serial/localization/battery probes
begin at AS frame 530 and Neo frame 120, while the new core groups begin at
540/150. Starting the observer later loses real early dispatches even when the
behavioral suite passes. The complete gate now checks coverage separately for
each live ROM, preventing another suite or model from hiding that omission.
