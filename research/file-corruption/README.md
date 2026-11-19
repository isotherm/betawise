# File size corruption bug (#6)

This is investigation of corruption of the high (upper 8) bits of the 32-bit
size, cursor offset, etc. in file info structures. This results in file loss
unexpectedly. Anecdotal reports include that this happens when using named
files, or applets that store their own files (e.g. KeyWords or MathFacts),
but other users who use all of those features have not seen the problem.

Postulated possible causes to investigate:
* [x] Intentional, direct modification by file routines
  * **RULED OUT.** Only low 8 bits of file flags are written.
* [X] Bugs in file info iteration, which provide the wrong struct offset
  * **RULED OUT.** Even with wrong offset, file calls don't write high bits.
* [ ] Applet modification by returned indirect pointer
  * **IN PROGRESS.** Verify any use of pointer from `FileAccessName`.
* [ ] Overflow from neighboring RAM structures

## AS3000 syscalls that set bits in FileInfo_t

Static inspection of AS3000 System 3.01 (ROM SHA-256
`732dca63399d883071ce41cc07ad352fa19c506b7b8c307ace16d5c53e6f8708`;
addresses are absolute ROM addresses) identifies these direct bit-setting writes:

| Syscall | Entry | Write instruction | Effect |
|---|---|---|---|
| A1C8 / slot 114, `FileOpen` | `0x004E1B18` | `0x004E1C20`: `bset #1,35(a3)` | `file->flags |= 0x00000002`. Offset `0x23` is the low byte of the big-endian 32-bit `flags` field at `0x20`. |
| A1D0 / slot 116, `FileUpdateFlags` | `0x004E1C50` | `0x004E1C8C`: `or.l d7,32(a4)` | When `set == 1`, `file->flags |= mask`. |

`FileOpen` first clears bit 1 in matching entries of the active applet table
(`0x004E1C12`: `bclr #1,35(a4)`), then sets it in the resolved entry. The
matching test uses `file_id & 0xFF00` against the active user byte shifted left
eight when that byte's bit 7 is clear; when bit 7 is set, both comparison values
are zero and every entry in the active table matches. This is the fallback-current
flag, not a write to the opaque byte at `0x1F`.

`FileUpdateFlags` requires `(mask & 0x0D) != 0`; otherwise it returns `-14`
after a successful file lookup. This validation does **not** restrict the applied
mask to `0x0D`: the full 32-bit mask is ORed, so a mask such as `0x03` can also
set bit `0x02`. Values of `set` other than exactly 1 take the clear branch,
`file->flags &= ~mask` (`0x004E1C96`). A1D4 / `FileTestFlags` only tests bits;
it does not set them.

These are direct writes in the syscall implementations, not an inventory of
every caller that can reach them. Instruction bytes were checked against the
local ROM disassembly; slot mappings are in the shared
[slot table](../abi-audit/slots.tsv), and the field layout is in
[`os3k_file.h`](../../os3k/os3k_file.h).

## Syscall access to a live FileInfo_t pointer

No identified AS3000 syscall returns the base of a `FileInfo_t` directly.
However, A1F0 / slot 124, `FileAccessName`, returns a pointer to the entry's
inline `name` field, so an applet can recover the containing struct's address.
Passing `NULL` for the name buffer and zero for the rename argument obtains
that pointer without copying or renaming:

```c
#include <stddef.h>
#include <stdint.h>
#include "os3k_file.h"

char *name = FileAccessName(file_id, NULL, 0);
FileInfo_t *info = name
    ? (FileInfo_t *)((uint8_t *)name - offsetof(FileInfo_t, name))
    : NULL;
```

In the same AS3000 3.01 ROM, the implementation starts at `0x004E26FA`.
Its successful return at `0x004E28B0` executes `moveq #52,d0`, followed by
`add.l a6,d0` at `0x004E28B2`: the resolved entry address plus `0x34`, the
offset of `name`. Lookup failure returns zero. The public SDK bridge converts
the ROM's D0 pointer result to the C ABI's A0 pointer result.

For comparison, A1C4 / `FileBindState` returns the file's backing-buffer
address, and A220 / `ClipboardGetData` returns the clipboard backing-buffer
address; neither returns the metadata entry. The internal resolver at
`0x004E366E` does return the entry base, but has no syscall table entry.

The recovered pointer refers to live firmware metadata, not a copied record
or a dedicated public struct-access API. File-table compaction after deletion
can invalidate it; resolve it again before subsequent use. These findings come
from static ROM inspection, rather than a new runtime probe.
