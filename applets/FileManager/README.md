# File Manager

Prototype SmartApplet `0xA1F1`. Build with `make -C applets/FileManager` from
the Betawise root. The recursive applet build discovers this directory.

Choose an installed applet with Up/Down and Enter. Its existing file table
appears as a folder. Up/Down selects a file; R renames, M moves, D deletes,
and Escape returns to the applet chooser. Names are limited to 15 bytes.
The file view shows the ID, four-digit hexadecimal tag, and logical byte size.
Delete and move require confirmation. Mutations use the native password prompt
when file password protection is enabled.

## Storage model and evidence

Folders are applet-owned file tables selected with `FileSetFolder`, not nested
directories. Folder selection is independent of applet identity and execution.
The manager restores the previous file folder after each event. System context
zero and the global clipboard are deliberately excluded from the browser.

The tag is **16 bits**, stored at offset `0x46` of the `0x48`-byte file entry.
It is distinct from the 16-bit file ID at `0x44` and from the byte-sized group
argument of `FileCreate`. `FileAccessTag` reads/writes a word, and
`FileFindByTag` compares that word within the selected applet table, with an
active-user filter. No universal extension/type registry has been established.

Stock firmware uses tags as stable record/shortcut identifiers. In the local
AS3000 ROM disassembly, `0x4C235A` stores a zero-extended byte from the shortcut
mapping table as a tag; `0x4C1D3A` and `0x4C2260` look up that mapping through
`FileFindByTag`. The native password dialog reads the current file tag at
`0x4C8D64` and compares it with File-key codes immediately afterward (for
example `0x2D`, File 1). The tag services themselves are at `0x4E28BE` and
`0x4E2902`. These observations establish a shortcut role for these callers,
not a contract for every proprietary applet. The current Betawise applet C
sources contain no other tag API callers. Do not reinterpret or rewrite tags
as file extensions.

There is no public enumeration call. The prototype probes exact nonzero IDs
through `FileResolveId`, 128 per idle event, skipping clipboard and size-query
selectors. It keeps up to 255 IDs (the inspected folder count is byte-sized).
It does not assume a particular high/low byte partition or read ROM-specific
RAM table addresses. Full enumeration costs up to 65,535 probes per refresh;
latency on physical hardware remains to be measured. Entries appear as found.

## Move contract

Moves currently support ordinary, unflagged files. They preserve logical bytes,
name, tag, password, and sequential position, and assign a new destination ID.
They reject any file testing positive for flags `0x01`, `0x04`, or `0x08`, whose
broader semantics remain unresolved; an API error also rejects the move.
They reject a destination tag collision rather than overwrite an existing file.

The destination is allocated first. Each 128-byte chunk is copied and read back
for comparison, then the destination length is checked. Only afterward is the
source deleted. Copy/verification failure deletes the partial destination and
retains the source, restoring its position. Cleanup failure is reported; source
delete failure leaves both copies and reports that outcome. Empty files use
the same path. This is not an atomic or power-loss-journaled transaction.

Saved/undo length, retained bytes past EOF, allocator reservations, opaque flags,
and live mirror bindings are not transplanted. Copying into another applet does
not convert its data format or ensure that the receiving applet understands it.
Special editor/linked files need an applet-specific move contract before support
can be enabled. Rename and delete remain native file API operations; deleting a
stock applet's expected record may change that applet's behavior.

## AlphaWord as a source editor

For writing Python in AlphaWord, prefer an explicit **Run from AlphaWord** or
**Import from AlphaWord** operation in MicroPython over moving the native record.
Running reads a snapshot while leaving the original editable in AlphaWord;
importing creates a Python-owned source copy with a destination-specific tag.
Neither operation should carry AlphaWord's shortcut tag into the Python folder.

The import path needs a bounded native-to-plain-text conversion that preserves
indentation, normalizes line endings, and handles native character encoding and
formatting tokens explicitly. It must not silently change code characters or
strip significant whitespace. The existing clipboard plain-text conversion is
evidence for conversion behavior, but its documented output-limit concern means
it is not yet a suitable general import implementation.

MicroPython currently executes a 511-character REPL input buffer using
`MP_PARSE_SINGLE_INPUT`; it does not load stored script files. Supporting this
workflow requires a script loader and `MP_PARSE_FILE_INPUT` execution, with
source naming for tracebacks and explicit memory limits. This is proposed work,
not functionality provided by the manager's raw move operation.

## Later phases

1. **Complete metadata and special-file handling.** Establish native enumeration,
   reserved-record rules, saved lengths, and receiving-applet import policies.
2. **Flash backup/restore.** Identify writable flash regions and erase geometry,
   firmware ownership, programming entry points, and interrupt/execution rules.
   Store a versioned archive containing applet ID (not transient table index),
   file ID, name, tag, metadata, raw bytes, lengths, and checksums. Commit a new
   complete archive before retiring an old one; reject incomplete/checksum-invalid
   archives. Test interruption during programming and erase. No flash writes are
   implemented in this version.
3. **USB object transport.** MTP maps naturally to applet folders and file objects
   without exposing the allocator as a disk. First establish whether an applet
   can control device/configuration descriptors, endpoints, setup requests,
   transfer completion, and disconnect. Existing USB messages are still opaque;
   their presence does not prove an applet can replace the ROM USB function.
   A firmware change may be needed. Stage uploads until complete, resolve name
   and tag collisions, and serialize host mutations with device-side editing.
   Keep raw backup objects separate from converted text exports.

MTP can use Windows' built-in driver ([Microsoft hardware vendor guidance](https://learn.microsoft.com/en-us/windows-hardware/drivers/portable/guidance-for-the-hardware-vendor)).
That does not guarantee software-free transfers on every desktop platform;
macOS commonly needs an MTP client ([OpenMTP](https://openmtp.ganeshrvel.com/)).
If native file-manager support across platforms is essential, evaluate a
**synthetic** USB mass-storage volume too: its FAT sectors would be generated
and imported through staging, rather than exposing System 3 RAM as a block
device. This remains a design option, not an implemented transport.

## Validation

The cross-build validates compilation, linkage, relocation fixups and image
generation. AS3000 MAME native cold boot, applet selection, AlphaWord file
listing and tag lookup, committed rename, and delete confirmation/cancellation have been exercised
after fixing the odd-address stack initializer. See the
[address-error investigation](../../research/file-manager/README.md) for the
reproduction and extended validation workflow. Moves, full allocator/password
policies, NEO and physical hardware remain unqualified.

The experimental B3 flash driver is private to this applet: `Flash.h`, `Flash.c`
and `FlashWorker.S`. Both File Manager and its research probe use the standard
SDK linker script and CRT. The self-contained worker is ordinary ROM code,
copied explicitly into a temporary, aligned stack buffer only during an
operation. There is no custom linker script, special RAM-code section or
in-image archive reservation; unused driver code is discarded when linking.

File Manager's UI does not yet call the driver or reserve/program archive flash.
The [archive design report and hardware probe](../../research/flash-b3/README.md)
compare full snapshots, append-only backup and TI-like file archives, including
erase geometry, wear and shutdown latency. The probe uses the same private
driver. Production `FlashOpen` refuses storage until a firmware-enforced archive
partition policy is implemented. Only the probe build authorizes the virtual
region supplied by its emulator fixture.
