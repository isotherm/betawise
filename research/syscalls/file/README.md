# File tables, passwords, linked text, and clipboard

## Scope

A190–A228, `FileInfo_t`, allocator reservations, and text-format conversion.

Caller contracts remain in [`os3k_file.h`](../../../os3k/os3k_file.h). This page records
**evidence and confidence**, not a duplicate normative API reference. No new
reverse-engineering claims are inferred from the documentation reorganization.

## Findings and open questions

| Finding | Status and evidence |
|---|---|
| 0x48-byte file entry, mirrors, position/length/capacity | ROM traces and historical RAM/file probes. Mirrors are addresses of caller-owned variables, not duplicate inline metadata. |
| Offset 10 reservation | Allocator minimum-retained length, separate from logical/saved length. Historical lifecycle scripts preserve the original fixtures. |
| Offset 1F | **Unresolved reserved byte.** No portable policy established. |
| Flags 01/04/08 | Mask validation/update mechanics established; broader policy meaning remains **incomplete**. Do not infer read-only/selection/source-file meanings from the names of operations. Bit 02 participates in current-file fallback. |
| Linked `<FORM>` content | Receive-side conversion creates internal marker sequences; view state was separate from these entry fields in the traced paths. This is not a claim that every clipboard/linked-file interaction is understood. |
| Clipboard plain versus raw conversion | Marker stripping and byte conversion observed. Plain `ClipboardGet` had a stack-dependent limit concern; do not treat that exploratory result as a fully verified bounded-copy contract. |
| Password reset / defaults | Historical native-dialog and candidate-password probes are retained. Distinguish accepted master-password paths from unverified policy assumptions. |
| File IDs and applet ownership | IDs resolve in selected applet tables; do not assume a universal 256-files-per-type scheme from a 16-bit token alone. |

The three file/clipboard pointer-return services still need the bridges listed
in the shared ABI audit. This reorganization does not silently repair or
recertify them. Native installation/persistence is distinct from a seeded RAM
record used to test a resolver or pointer return.

## Tests and reproduction

The maintained ABI runner checks pointer conventions using controlled file records. No maintained full allocator, password-dialog, or linked-text workflow suite is supplied. Earlier observations in this README are not fresh execution claims.

The maintained combined runner, executed from the workspace root, is:

```sh
python3 betawise/research/abi-audit/run.py /tmp/os3k-syscall-validation
```

Only the cases explicitly described here are covered; running that command is
not blanket certification of this entire subsystem. The shared
[ROM inventory](../../abi-audit/rom-inventory.json),
[slot table](../../abi-audit/slots.tsv), and
[ABI audit](../../abi-audit/README.md) define firmware/ABI scope.

## Slots 146–150: protection, master password, and users

A248/A258 get/set a shared protection byte (AS 4472, Neo 3.21 118A2).
Control Panel's original caller ties it to the file-password ON/OFF setting;
these calls themselves neither check passwords nor rewrite file entries.
A24C compares the separate master-password string case-insensitively; A250
prompts with masked six-byte input and retries until a match or Escape.
A254 reads four 30-byte user-name slots (AS 3D7A, Neo 3E0A); slot zero resolves
the active slot from the low seven bits of the selection byte. The ROM does
not validate an unset zero active slot, so that input is excluded from the API.
These are profile indices, not an applet/file ID partition claim.

See the [shared integration investigation](../irda/README.md) for runnable probes,
exact coverage, and remaining transport/UI limits.

## Native editor and forms follow-up

Slots 175–194 consume file flag 04, maintain a 512-byte encoded gap, and use a separate clipboard file for form control editing. These are new consumer observations, not a complete naming of file flag 04. See [text research](../text/README.md).
