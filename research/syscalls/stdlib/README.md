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
[abi-audit](../../abi-audit/README.md). The [legacy stdlib group](../coverage/README.md)
adds scalar/stream calls, parsing, formatting, random seeding, byte conversions,
and non-returning abort control transfer on both live ROMs. Historical scrolling
observations are not an exhaustive maintained scrolling regression.

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

## StringCompare byte limit (2026-11-19)

Original AS3000 3.01/Neo 3.21 code reads both option arguments as bytes. Count
zero becomes FF, and FF is the unbounded sentinel: 0 and 255 compare through
NUL; 1–254 limit comparisons. A raw 256 therefore behaves like zero. The result
is the low-byte subtraction, with wrap, rather than a guaranteed -1/0/+1.
The public prototype now uses uint8_t options and int8_t result. Maintained
cases check one-byte limits, both sentinels, truncation of a raw wide argument,
and case folding. Neo entry 41C900 reads SP+33/SP+37 after saving registers and
uses FF as the nondecrementing limit; the AS implementation agrees in live tests.
The Neo function was rechecked in Ghidra with analysis disabled. The maintained
[inspector](../wireless/InspectNeoTail.java) accepts 41c900 as an explicit address
to reproduce the instruction/decompiler view.
