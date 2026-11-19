# Elapsed time, delays, and timers

## Scope

A0D4–A0EC and investigation of neighboring keyboard slots.

Caller contracts remain in [`os3k_time.h`](../../../os3k/os3k_time.h). This page records
**evidence and confidence**, not a duplicate normative API reference. No new
reverse-engineering claims are inferred from the documentation reorganization.

## Findings and open questions

| Finding | Status and evidence |
|---|---|
| Clock separation and timer mechanics | ROM traces plus earlier counter/slot/flag probes; five one-shot slots, minimum/range behavior and stale handles. |
| BusyLoop timing | Historical AS MAME run: 10,000 iterations about 66 ms at the configured 16 MHz. Instruction timing estimate about 106 cycles/iteration. This is not a portable delay unit. |
| Timer callback A5 | Timer handler lacks an applet-base switch; do not infer safety from an absolute-address callback fixture. |
| Cancellation/service stop | Different notification behavior; lifecycle and callback storage must be considered separately. |
| Focus-loss gating | **Unresolved.** No claim that kill-focus always prevents callbacks, or that callbacks necessarily continue after it. |

The maintained fixture uses BusyLoop and tests original timer-service
notification expiry and cancellation. Physical timing, callback A5 ownership
after focus changes, and every firmware version remain unqualified.

## Tests and reproduction

The [legacy time group](../coverage/README.md) checks busy-loop returns,
RTC/timer counters, delayed progress, range rejection, expiry flags, and
cancellation/stale handles, callback event/A5 capture, and absence of cancelled
callbacks beyond their deadline on AS3000 3.01 and Neo 3.21. The serial integration
suite additionally exercises timer-backed waits. Neither certifies focus cancellation.

The maintained combined runner, executed from the workspace root, is:

```sh
python3 betawise/research/abi-audit/run.py /tmp/os3k-syscall-validation
```

Only the cases explicitly described here are covered; running that command is
not blanket certification of this entire subsystem. The shared
[ROM inventory](../../abi-audit/rom-inventory.json),
[slot table](../../abi-audit/slots.tsv), and
[ABI audit](../../abi-audit/README.md) define firmware/ABI scope.

## Additional investigation details

These observations retain their original firmware and test scope; tentative
interpretations are not additional API guarantees.

System 3 syscall `0xA0D8`. Runs a fixed instruction loop iterations times.
This does not use a clock, sleep the CPU, or pump applet messages. Timing
depends on CPU speed and interrupts. In AS3000 3.01 MAME at its configured
16 MHz, 10,000 iterations took approximately 66 ms.
