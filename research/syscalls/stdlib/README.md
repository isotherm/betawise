# String helpers, standard library, and stream behavior

## Scope

Early helpers 27–31 and A32C–A3B0; Betawise console wrappers.

Caller contracts remain in [`os3k_stdlib.h`](../../../os3k/os3k_stdlib.h). This page records
**evidence and confidence**, not a duplicate normative API reference. No new
reverse-engineering claims are inferred from the documentation reorganization.

## Findings and open questions

| Finding | Status and evidence |
|---|---|
| Early strlen copy and binary-search helpers | ROM comparison and earlier typed probes; pointer bridges for range/pattern helpers already exist. |
| Unchecked numeral formatter | Ten-digit stack buffer; low bases can overflow it. Public checked wrapper rejects unsafe radices. |
| Standard-library pointer returns | All ten libc pointer-return adapters are implemented in os3k_abi_bridge.c, along with the early range/pattern helpers and other subsystem bridges. Compiled QEMU calls disable builtins and test arguments and NULL/non-NULL results. |
| Variadic formats | Maintained original-ROM tests support ordinary int/long/double cases and demonstrate unsupported `ll`; not a full modern-C dialect certification. |
| Console wrappers | Character-at-a-time Betawise output handles cursor/scroll state; historical scrolling probes are separate from system stream implementations. |
| Slots around stdlib | Old-table boundaries/holes are in the ROM inventory; A3B4 is not an AS3000 3.01 stdlib entry. Do not infer safe calls from adjacent bytes. |

Earlier stdlib logs show raw D0 return observations. Those do not repair the
pointer-return ABI and cannot alone certify every typed call or stream path.

## Tests and reproduction

Maintained pointer/aggregate/variadic probes remain shared in
[abi-audit](../../abi-audit/README.md). String/stream/scroll observations are historical; no maintained standalone scrolling probe is supplied.

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

Case-insensitive mode folds ASCII A through Z and the ROM's additional
encoded character pairs, such as 0x80 to 0x84 in the tested ROM. It is a
byte encoding conversion, not Unicode or a host-locale comparison.
