# Applet lifecycle, dispatch, and image headers

## Scope

Messages, `AppletHeader_t`, and applet identity/lookup services, particularly slots 139–145.

Caller contracts remain in [`os3k_applet.h`](../../../os3k/os3k_applet.h). This page records
**evidence and confidence**, not a duplicate normative API reference. No new
reverse-engineering claims are inferred from the documentation reorganization.

## Findings and open questions

| Finding | Status and evidence |
|---|---|
| Applet identity context is separate from foreground/A5/file folder | Ghidra traces and controlled descriptor fixtures in historical AS3000 3.01/Neo 3.21 MAME probes. |
| Select accepts an index only when its status word is zero | ROM branches and return/unchanged-context probes; index zero is ambiguous as a return value. |
| Message parameter widths and header offsets | ROM dispatch sites, manufacturer headers, linker/startup layout, and compiled ABI assertions. |
| `MSG_SETFOCUS` parameter | Entry offset seen at a traced site; header now treats it as opaque rather than promising that interpretation universally. |
| USB messages and reply codes | **Incomplete.** Payload block layout, nonzero replies, and transfer-event policies are not established. Opaque declarations are retained without a speculative payload structure. |
| Header trailing words | Betawise emits 0/1/2; their firmware policy meaning is not established. Preserve the initializer. |

Historical AS3000 3.01 table/status bases: D82 / 344E. Neo 3.21: E0A / 34DE.
The identity cache uses 3560 / 35F0 in the original fixtures. Native Neo MAME
boot lacked a usable system descriptor in some runs, so controlled RAM headers
were seeded. These checks are not full applet installation/launch tests.

Whether kill-focus prevents every outstanding timer/UART callback remains an
open lifecycle question. The inherited-A5 finding does not prove either that
callbacks continue after focus loss or that the firmware cancels them correctly.

## Tests and reproduction

Shared aggregate/header ABI probes remain under abi-audit. No maintained comprehensive applet lifecycle fixture is supplied; lifecycle observations here are historical.

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

The entry point returns void. Use the status output for the reply requested
by the message. Write zero for ordinary events handled without a special
reply. The status word can contain a previous value on entry.
The USB request block and nonzero USB reply codes remain partly unresolved.

< Applet gains focus. param is an opaque launch value, observed as the entry offset. Reply zero for ordinary handling.

< USB event. param points to a USB request block. Reply semantics are unresolved.

< USB detach. param points to a USB request block. No reply is read at the traced call site.

< USB transfer event. Payload and reply details are unresolved.

< 0x88, three trailing words. Betawise emits 0, 1, 2. Their ROM semantics are unresolved.

## Slots 152–156 and 158–161: masks and resources

The 32-word mask array is at AS 344E / Neo 3.21 34DE. Getter/set-system-bit/
clear-all have no bounds checks. Context-lock add/remove accept targets 1–31
and use bit `1 << current identity`, sourced from AS 3552 / Neo35E2's low byte.
They do not register message subscriptions or switch A5. Native disable UI
uses bit zero and the string “This SmartApplet has been disabled.” Other bits
retain their context-lock interpretation; not every lifecycle consumer was tested.

Slot 158 resolves header offset 0C as a resource-stream pointer. The existing
header member settingsOffset is retained for source compatibility, with its
meaning clarified. The pointer return is in D0 only, now bridged in
os3k_abi_bridge.c. The six-byte resource header is type/id/length, all 16-bit,
followed by payload and even padding. Type zero terminates searching.

159 does not advance on length zero; 160 and successful 161 do advance by six.
The odd-length rounding is 16-bit and wraps at FFFF; supported callers must
stay at or below FFFE. Reader outputs are all mandatory; search outputs are
optional and preserved on failure. Writer does not initialize padding or append
a terminator. No allocation or boundary checking is supplied by these calls.

The [shared integration investigation](../irda/README.md) records Ghidra targets,
raw cases, compiled-C bridge checks, and limits.

## Native text message handlers

The [text investigation](../text/README.md) shows that slots 176/189 take full applet messages, while 186/191 also use transport prefixes such as 60001 and 60020. Their payload layouts are only partially mapped; no speculative universal USB/print request struct was added.

## Installation and registration

See [Manager finalization and warm registration](../../applet-installation/README.md)
for AS3000 3.01 install-transaction findings and the limits of AppletSendMessage
for registering a newly appended applet.
