# Flash bus width and USB repair boundaries

Read-only source/datasheet inspection and disposable NEO13 control-flow probes,
2026-11-20. No physical programming or erasure and no successful USB reflash
are claimed.

## Part capabilities versus board wiring

| Family | Byte-wide parts (x8) | Word-capable counterparts |
|---|---|---|
| AMD | Am29LV004B, Am29LV008B | Am29LV800B: x8/x16 |
| ST | M29W004, M29W008 | M29W800A: x8/x16 |
| Fujitsu | MBM29LV004, MBM29LV008 | MBM29LV800: x8/x16 |
| Micron | MT28F008B3 | MT28F800B3: x8/x16 |
| Intel Advanced Boot Block B3 | 28F008B3 | 28F800B3: x16 |
| Older Intel SmartVoltage | 28F008BV/BE | 28F800BV/CV/CE: x8/x16 |

These are device capabilities from the manufacturer documents linked in the
[device catalog](updater-chip-geometries.md), not a measurement of fitted-chip
wiring or active mode. A CPU MOVE.W/MOVE.L does not establish the flash's
programming width, command atomicity, or even a single external bus operation.

## USB is a transport, not a raw flash programmer

`neotools/neotools/message.py` names block-upload command 02, program-buffer
command 0B, clear-segment-map 16, erase-segment 17 and SmallROM-updater request
18. Block upload alone does not select an arbitrary flash address. Firmware
protocol state, current destination, segment descriptors, chip command support
and protection determine what is programmed.

In NEO13 SmallROM the packet handler at 401A80 dispatches command 17 to 401C24.
That path records up to four destination/length segments and invokes RAM erase
wrapper 4022B8. Command 0B at 401DB4 programs buffers through RAM wrapper 402262
and advances through the configured segments. The observed handler supports
addressed firmware segments, rather than just normal applet append. This is not
a complete range-validation audit or proof that every physical block is writable.

The current neotools installer explicitly rejects non-applet ROM images before
transfer, stating ROM installation is untried. Vendor Manager's ability to update
SmallROM does not prove the currently executing SmallROM updater can erase and
rewrite its own executing sector. The inspected small worker wrappers return
to SmallROM after programming/erasing; self-update needs a separately verified
RAM-resident or full-ROM-resident orchestration path. Merely finding RAM flash
workers does not establish that complete path.

## Failure of a handoff address versus failure of updater code

The candidate patches alter only the operand of the final normal-system JSR,
not the reset vector, key scanner, password routine, protocol handler or flash
workers. In NEO13 the manual recovery decision precedes that JSR: 401252 checks
the service chord, 40125A calls the password routine, and accepted entry calls
40148E then 401730 before the normal path can reach 40143E. This is a static
reason for recoverability if only the intended operand becomes invalid.

The service chord decoded from the native key checks is Right Shift + Comma +
Period + Slash; the password scan-code array spells ERNIE. The familiar Left
Shift + Tab is not sufficient in these fixtures: both the stock and broken
handoff cases still took the normal handoff. Do not confuse normal firmware
updater mode with SmallROM's independent recovery path.

A deliberately unusable normal handoff can prevent normal boot while leaving
these earlier instructions unchanged. Reaching SmallROM recovery and completing
a USB repair are separate acceptance criteria. Physical brownout behavior and
whether an interrupted program affects only the expected byte/word still need
the actual chip contract. General NOR interrupted-operation guidance does not
justify an unconditional no-brick guarantee.

See `recovery-hook/fallback/` for the fixture runner. It uses a copy of a stock
fixture from `recovery-hook/run.py`, never saved user NVRAM. These tests observe
CPU/memory control flow, not UI text, and do not exercise USB transport or erase
any sector.

## Executed fallback observations and present limit

On disposable NEO13 images, both stock and a handoff changed to the unusable
405F98 target reached the original SmallROM service-password routine when the
service chord was held. The broken target without successful forced entry
prevented normal boot (sampled PC 0008E698). This supports independence of the
password gate from the modified handoff operand.

The automated password-entry attempts did NOT establish arrival in updater
entry 401730 or USB setup 402EC4. Longer attempts also failed to establish those
endpoints. These are incomplete/inconclusive fallback tests, not repair PASSes.
A changed operand being outside those routines does not prove that physical
power failure leaves all required bytes reliable, nor that a self-reflash can
complete. The release gate remains an end-to-end demonstration: deliberately
unusable handoff, manual SmallROM entry, real USB dialogue, restoration of the
original image through the vendor-supported path, and normal subsequent boot.
Repeat for each supported model/revision before relying on this as a no-brick
installation strategy. No erase/program commands were sent by these probes.
