# Self-contained SmallROM recovery placement

Read-only package/encoding inspection, 2026-11-20. No physical flash changes,
no tail writes, and no claim that an omitted package range is physically empty.

## Candidate tails in all three available SmallROM images

| Image | JSR address | Current destination | Minimum-bit candidate | AND mask | Cleared bits | 0-to-1 bits | Tail bytes after candidate |
|---|---|---|---|---|---:|---:|---:|
| AS3000 | `00400BD8` | `004FFF98` | `00403F98` | `FFF03FFF` | 6 | 0 | 104 |
| NEO 2005 | `00401328` | `005FFF98` | `00405F98` | `FFE05FFF` | 7 | 0 | 104 |
| NEO 2013 | `0040143E` | `0046FF98` | `00405F98` | `FFF95FFF` | 4 | 0 | 104 |

Each candidate was checked directly against its original six-byte JSR encoding:
`old & AND_mask == new`, `old & new == new`, and `new & ~old == 0`
(with 32-bit masks). The opcode is unchanged. Thus every changed bit is 1-to-0;
no 0-to-1 operation is required. Exhaustively enumerating even destinations
with room for a six-byte trampoline in each omitted tail gives minimum counts
6, 7 and 4 respectively. There is no one-bit tail destination for these three
handoff instructions. This is not a proof about every possible instruction
patch or other callsite in the ROM.

The earlier targets 403A98, 404700 and 405880 also satisfy the bit-clear rule
but clear 8, 12 and 9 bits. The new targets trade initial contiguous space for
fewer changed bits: 104 bytes can hold a trampoline into the larger earlier
part of the same tail, if that entire area is qualified as available.

These changes affect two address bytes on opposite sides of a 16-bit word
boundary. For example, the AS3000 operand is `00 4F FF 98` -> `00 40 3F 98`:
words `004F` and `FF98` both change. An x8 flash needs at least two byte program
operations; an x16 flash needs at least two word program operations for this
patch. A 32-bit CPU store does not make those operations atomic. Use the chip's
programming command sequence, not an ordinary CPU AND write to mapped flash.

Even within one byte/word program operation, the examined datasheets provide
no power-loss atomicity guarantee. Internal programming applies pulses and
verifies cell margins; interruption can leave incompletely programmed or
unreliable cells. Do not assume an all-old/all-new word, ordered bit clearing,
or a read-stable subset of intended changes. Infineon/Cypress manufacturer
[recovery guidance](https://community.infineon.com/t5/Knowledge-Base-Articles/Recover-Flash-Devices-when-Power-Failure-or-Reset-Happens-During-a-Program-or/ta-p/248567)
explicitly treats interrupted-operation data as untrustworthy; this is general
NOR guidance, not an atomicity specification for the old Intel/Micron parts.
Micron's [MT28F008B3/800B3 datasheet](https://media.digikey.com/pdf/Data%20Sheets/Micron%20Technology%20Inc%20PDFs/MT28F(008,800)B3.pdf),
p.7, describes byte/word bus selection and per-cell write-margin verification.
A partially programmed jump may select an invalid intermediate destination.
No boot or physical-programming qualification follows from the encoding check.

The complete omitted tails are 1,386 bytes after AS3000 SmallROM (ends 403A96),
6,490 after NEO05 (ends 4046A6), and 2,022 after NEO13 (ends 40581A). The upper
bounds above come from the next known package destination, not from measured
physical blank space or an erase-sector boundary. Physical flash contents,
board-specific data, updater staging uses, integrity checks and all references
must be checked before using them. These are all local available revisions;
they do not establish coverage of every SmallROM release ever shipped.

Actual FF runs inside the supplied SmallROM images are much smaller. AS3000 has
only a seven-byte run at 4036C7; NEO05 a twelve-byte run at 403CEC; NEO13 has runs
of 16 bytes at 4003F0, 8 at 400418 and 12 at 404DDE. FF does not prove non-use:
these can be vectors, identity or metadata fields. In particular NEO13 code
references 400418, so that eight-byte field is not an unused code cave.
No suitably sized, qualified internal blank area was established.

Reproduce address/bit checks and source hashes:

```sh
python3 research/flash-b3/smallrom-space.py
```

## Why prefer recovery-local code over a full-ROM cave

A persistent SmallROM hook that points to a stub in normal firmware becomes a
cross-update dependency. A normal firmware update may erase/replace the stub
while leaving SmallROM's changed jump intact, causing the next handoff to enter
missing or unrelated instructions. The original vendor updater might remain
recoverable through another path, but that has not been qualified; do not call
this configuration update-safe or assume the machine can boot normally.

Prefer a small self-contained bootstrap with SmallROM, provided the tail is
confirmed available. It must not call private routines in the replaceable full
ROM. It should validate transaction metadata and a versioned recovery capsule,
copy the complete flash worker and constants to safe RAM if needed, and preserve
the vendor handoff on the normal no-transaction path. A larger updateable capsule
can be stored redundantly in managed archive space if the fixed bootstrap can
find a valid copy after relocation; this still requires a proven persistent
location/selection protocol, not an assumed free last physical block.

A SmallROM update is a separate event: erasing its sector can remove both the
hook and bootstrap, or leave only a partially installed combination after power
loss. Keeping code beside SmallROM does not preserve our recovery through vendor
SmallROM updates automatically. Qualify a stock-handoff fallback and document
when the feature must be reinstalled. Never begin applet relocation while the
bootstrap/capsule arrangement is unavailable or a firmware update is in progress.
The active-transaction case needs explicit protection against replacing the
firmware or capsule on which completion depends.

The known main-ROM FF region on NEO13 remains an alternative for experiments,
but its verified byte values do not resolve this update-lifetime problem.
