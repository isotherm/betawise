# Speech queue and controls, slots 282–283

## Scope and evidence

Fresh original Neo 3.21 Ghidra disassembly/decompilation, manufacturer-app caller
scan, and live MAME fixtures establish these contracts. AS3000 3.01 and Neo 3.01 have
237 dispatch entries: these slots are outside their tables, not empty entries.
The exact images and hashes are in the shared [ROM inventory](../../abi-audit/rom-inventory.json).

| Slot | Opcode | Target | Clean-room name | Contract |
|---:|---|---|---|---|
| 282 | A468 | 00434B14 | SpeechEnqueue | Four 32-bit stack slots: low-byte notification flags, text pointer, callback pointer, opaque value. Low-byte result 0 accepted, 1 disabled, 2 full. |
| 283 | A46C | 00435064 | SpeechControl | One stack slot, low-byte operation. Low-byte result depends on operation. |

Public contracts and enums are in [os3k_speech.h](../../../os3k/os3k_speech.h).
No original SDK names were used. Original localized strings resolve to “Push
Enter to turn text to speech ON”, “Push Enter to turn text to speech OFF”,
“Read back Options...”, “Set voice speed...”, and “Set volume...”. These provide
independent identification of the subsystem, rather than just a UART-use guess.

## Queue record and lifetime

The private ring at 0x10C1C has 200 records, each 14 bytes, with two-byte
alignment. It is not passed by the public syscall, so no public structure type
is needed. Its observed layout is:

| Offset | Size | Meaning |
|---:|---:|---|
| 0 | 1 | Ring position/command identifier, 0–199. |
| 1 | 1 | Notification mask: 1 submitted, 2 advanced past item. Other bits are stored but not consumed by the traced notification sites. |
| 2 | 4 | Borrowed NUL-terminated text pointer. |
| 6 | 4 | Callback pointer, or zero. |
| 10 | 4 | Opaque callback argument. |

0x423 is the producer position; 0x422 is the consumer position. Each wraps at
200. The separate byte counters at 0x424/0x425 count enqueues/dequeues modulo
256. Their unsigned difference tests full at >=200 and empty at zero. An
active record has already been dequeued: the empty query does not mean idle.
The enqueue stores pointers, rather than copying text, and starts processing
when the processor state at 0x117EC is zero. Processing and callbacks can thus
begin before the enqueue returns.

0x434 is cleared on enqueue and set when processing becomes idle with no
pending items; it is distinct from the queue-empty query. Control 3 resets the
queue counters, processor, timers and transport state, optionally closing UART
and restoring a saved idle-sleep flag. Disable (1) merely clears the global
speech enable byte at 0x42C; it does not clear the queue.

The native processor at 0x434CA6 formats `%s%c%iI` commands, submits them to
0x435B80, and starts a 4000 ms timer. The lower command sender rejects strings
longer than 35 bytes. Enqueue does not enforce that length or report the later
transport error. The callback's event 1 means submission was attempted; event
2 means processing advanced, including timer-expiry paths. Neither proves
successful audible output. Text lifetime extends through active processing.

## Control operations

| Value | Action | Low-byte result |
|---:|---|---|
| 0 | Set speech enable byte to 1 | Previous enable byte |
| 1 | Set speech enable byte to 0 | Previous enable byte |
| 2 | Toggle bit zero of enable byte | Previous enable byte |
| 3 | Reset queue and active processor, clean up transport | Enable byte |
| 4, 5 | Invalid | FF |
| 6 | Set menu readback byte at 0x4350 to 1 | Previous readback byte |
| 7 | Clear menu readback byte | Previous readback byte |
| 8 | Cancel active speech, reset editor state, open speech settings | Enable byte captured before opening |
| 9 | Query enable byte | Current enable byte |
| 10 | Test pending queue empty | 1 empty, 0 nonempty |
| >10 | Invalid | FF |

Control 6/7 calls 0x41C40E. Its byte is consumed at 0x41B9FC while preparing
selected menu text for the speech enqueue at 0x41BBB8/0x41BBF4. This is menu
readback, not a global UART or keyboard sound flag. The high three argument
bytes are ignored. Higher return bits are not consistently initialized.

## Callback ABI

0x434CA6 calls the callback with two four-byte stack slots: event 1 or 2 and the
opaque value. The return is ignored. The function-pointer calls at 0x434E6E
and 0x434DF2 do not install callback-owner A5. Live assembly and compiled C
callbacks confirm argument order and inherited deliberately foreign A5. A
GOT-dependent callback must arrange its own correct context; the raw syscall
entry itself needs no return-register bridge. Do not infer kill-focus
cancellation or ownership safety from these fixtures.

## Manufacturer callers

The hash-checked scan finds six images exporting the validated contiguous
A468/A46C/A470 tail. Only Text2Speech Updater has resolved call candidates:
0x2B0 calls control 1 and saves its low-byte result; 0x5C8 and 0x5F2 call
control 0 only if the saved value was 1. Independent m68k disassembly confirms
all three indexed-PC targets resolve to the A46C stub at file offset 0x131C.
This disables speech during the updater and restores the previous enablement.
No direct 282 or 284 callers were resolved by this scan. This is not a proof
that indirect uses or other export layouts do not exist.

## Reproduction and test boundaries

From the workspace root:

```sh
python3 betawise/research/syscalls/speech/find-vendor-calls.py
python3 betawise/research/syscalls/speech/run-probe.py /tmp/os3k-extended-probe
```

`InspectExtended.java` can run as a Ghidra post-script against the original
Neo 3.21 image mapped at 0x410000; its default addresses include these services,
their consumers, and the separate wireless getter. No source-library symbol
matching is needed. Ghidra's inferred wide return types are corrected using
actual byte/word writes and typed GCC/MAME probes. The switch jump table in
283 must be checked along with its individual branch bodies.

The maintained runner tests enable/toggle/query values including noncanonical
bytes, invalid operations, menu-readback state, queue empty/full and counter
wrap, producer positions 0/1/199, borrowed pointers, notification masks,
start/finish callbacks, reset, localized menu labels, settings opening and
Escape dismissal, compiled public calls, and the separate [wireless status
fixtures](../wireless/README.md). It uses disposable NVRAM and original ROM
instructions. Callback fixtures deliberately cause the native transport length
guard to reject a long command, avoiding dependence on a speech peripheral.
They prove queue and callback behavior, not audible speech, successful serial
handshaking, or the safety of arbitrary caller buffer sizes. No physical
speech peripheral or wireless link was tested. Settings submenus were traced
but their hardware effects were not certified.

Validation on 2026-11-18: all 80 combined MAME cases passed, including
compiled scalar calls and start/finish callbacks. The library build, Doxygen
syntax/coverage check for both new headers, and local research-link checks
also passed. Generated HTML, logs, and binaries were kept outside the source
tree.
