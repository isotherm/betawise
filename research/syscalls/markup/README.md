# Restricted markup scanner, slots 271–276

## ROM scope and native contracts

All six are populated in Neo 3.21 and outside the AS3000 3.01/Neo 3.01 tables. Their
native ABI uses a six-byte aggregate at hidden A1: byte status at 0, padding
at 1, and a 32-bit offset at 2. Epilogues copy four then two bytes and leave
A0 = output + 4. Padding and many successful offsets are uninitialized.
These are not scalar/pointer returns and do not take an extra stack output
argument. [os3k_abi_bridge.c](../../../os3k/os3k_abi_bridge.c) calls the private native
stubs through GCC's compatible hidden-A1 input convention, copies semantic
fields, zeroes padding/success offset, and returns through GCC's normal C ABI.

| Slot | Target | Public wrapper | Stack arguments |
|---:|---|---|---|
| 271 | 442F86 | MarkupParse | begin, exclusive end, element array, byte capacity, filter callback |
| 272 | 443052 | MarkupGetAttribute | element, attribute name, output, signed word capacity, word length output |
| 273 | 44313A | MarkupGetText | element, output, signed word capacity, word length output |
| 274 | 443214 | MarkupFindElements | parent, relative path, element array, byte capacity, byte count output |
| 275 | 4432B0 | MarkupGetChildAttribute | parent, path, attribute name, output, **unused argument**, word length output |
| 276 | 44331C | MarkupGetChildText | parent, path, output, signed word capacity, word length output |

Slot 275 **hardcodes capacity 18** in its call to 272. The public wrapper
therefore requires eighteen bytes of output storage and hides the unused
legacy argument instead of advertising a working dynamic capacity.

## Borrowed element structure

Each sixteen-byte element contains four pointers: opening-tag attribute
begin/end, then content begin/end. Names are supplied through the filter path,
not separately retained in each record. Keep the original input storage alive.
Self-closing tags can have empty content spans. The parser has shared global
state and a ten-level stack, not a per-call reentrant context.

The filter receives path, current name, signed byte depth and signed byte output
index (four stack slots). The supplied default callback at 442F7A returns zero.
Thus **NULL validates syntax but retains no elements**. An The scanner writes
the proposed record before consulting the filter, so valid output storage is
required even with a NULL filter. An accepting callback returns a nonzero word;
compiled C confirms argument order and that paths begin with slash. It executes
synchronously with inherited A5.

Status codes found in the original branches include 0 success, 1 invalid
name-state transition, 2 syntax/entity error, 5 incomplete input, 6 depth limit,
7 name longer than twenty bytes, 8 element capacity reached, 9 mismatched
closing name, 10 absent root, 11 multiple roots, 12 empty attribute span and
13 output full. The public enum describes these branch meanings. Some error
states are static-only observations; the maintained fixtures explicitly
exercise the subset below. Successful native offsets vary with previous parser
state and must not be treated as a consumed-byte count. Wrapper normalization
eliminates that ambiguous success value and the native padding byte.

## Extraction behavior and limitations

Tested relative child paths begin with `/`, e.g. `/child`; neither `child` nor
`/root/child` matches the same child when searching within root's content.
Attribute matching is case-sensitive. A missing attribute can return success,
length zero and **unchanged output**. No child match can leave the output and
length untouched. A caller must inspect length/count, not just success.

Both extractors initially test the **attribute** span for emptiness. Consequently
273 can return 12 and an empty output even for an element with nonempty content
but no attributes. The live fixture confirms this quirk. Slot 272's empty-span
path leaves output and length untouched; 273's clears both. Text extraction
copies nested-tag bytes rather than stripping markup. It decodes amp, lt, gt,
apos, quot and decimal/hex numeric byte entities; numeric values over 255 are
rejected. Truncation NUL-terminates the copied prefix and returns 13, with 273's
native offset negative relative to the original content beginning.

The parser is not a general XML library. No full standards compliance,
untrusted-input memory-safety guarantee, or arbitrary encoding support is
claimed. Positive capacities and valid span ownership are required.

## Evidence and reproduction

```sh
python3 betawise/research/syscalls/markup/run-probe.py /path/on/disk/markup-probe
```

Fresh Ghidra traces cover all six entries and the scanner, open/close handlers,
path filter, attribute handler and entity decoder (4433A8, 4435B2, 443994,
4439BA, 443ABC, 443B34, 443388, 443414). The maintained
[tail inspector](../wireless/InspectNeoTail.java) covers the entry points;
additional helper addresses can be passed explicitly. No SDK symbol inference
was used. Vendor MathFacts RP, Accelerated Reader and KeyWords Wireless have
resolved parser caller candidates in the shared tail-stub scan.

Forty MAME cases pass, including document/error samples, accepted
borrowed spans, NULL-filter behavior, empty spans, entity and numeric decoding,
invalid entities, capacity boundaries, relative path matching, attribute/text
extraction, truncation and child convenience operations. All six production C
wrappers are linked from source and executed, with static size/offset checks,
a compiled filter, and assertions on normalized padding/status/offset.
Raw calls verify exact six-byte writes with surrounding sentinels and the
native A0 = output+4 value. Generated logs/binaries remain disposable.

The suite was repeated successfully after the Neo low-SRAM/high-MMIO mapping
correction (2026-11-18); no firmware target was patched for that regression.
