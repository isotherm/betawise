# Dormant firmware multi-user mode

## Decision

**Do not expose this feature in DebugTool, Control Panel, or the ordinary file
adapter yet.** AS3000 System 3.01 contains working portions of a named-profile
UI and grouped file storage, but qualification found two substantial defects:

1. The UI creates/selects a fourth profile, while `FileCreate` rejects group 4.
2. After switching from Alice to Bobby through the native selection/password
   flow, AlphaWord reopened Alice's group-1 document with its text intact.

These findings are sufficient to withhold a supported interface. They do not
establish why the vendor left the entry screen disconnected. Nothing here is
an implementation of multi-user support or a recommendation to enable it on a
user's device.

## What a profile/group means

Profiles are firmware-global, separate from applet-managed identities such as
KeyWords students. In AS3000 3.01 there are four 30-byte slots at `0x3D7A`:
name at slot offset 0 and password at offset 20. The native selector creates
these records; they are not files in an applet directory. The mode-enabled byte
is `0x3DF2`; `0x3DF3` holds selection state. Its low seven bits identify the
profile, while bit 7 marks an unselected/unfiltered state.

The group passed to `FileCreate` is encoded in the high byte of the file ID.
It is not the independent 16-bit tag at file-record offset `0x46`, an extension,
a content type, or a Unix permission group. Creation/deletion can renumber IDs.
Some firmware operations filter by profile; explicit file-ID resolution does
not enforce ownership. See [file API](../../os3k/os3k_file.h).

## Static findings and scope

Inspected local firmware identities are in the shared
[ROM inventory](../abi-audit/rom-inventory.json). Addresses below identify internal
routines, not supported public syscalls or qualified portable entry points.

| Firmware | Toggle UI | Profile selector | Startup profile-state helper |
|---|---|---|---|
| AS3000 3.01 | `0x4E00F8` | `0x4DFA3E` | `0x4DF9EE` |
| Neo 3.01 | `0x5E2A54` | `0x5E239A` | `0x5E234A` |
| Neo 3.21 | `0x438E52` | `0x438798` | `0x438748` |

No direct control transfer, absolute pointer, or recognized indexed A0 call to
the toggle UI was found in those three ROMs. This is not exhaustive proof that
no indirect route exists. No normal menu entry, shortcut, or enable syscall was
established. Searches of the six local vendor manuals found no instructions for
this profile mode. Their references to sharing via file passwords or KeyWords
students must not be treated as documentation for firmware profiles.

On AS3000 fresh initialization disables the mode and clears the profile slots;
normal initialization marks profile selection pending. The toggle UI checks
the master password, changes the mode, and abandons its invocation through
`0x4C0068` (the firmware's non-returning applet-control restart). It does **not**
behave like a normal returning dialog. Enabling entered the profile selector.
Calling the selector while a profile is already selected returned immediately.
There is no qualified ordinary logout/switch-user entry point.

AS3000 `FileCreate` at `0x4E1FD0` accepts group zero, or a nonzero group strictly
less than 4 whose profile-name slot is nonempty. The selector offers four
slots. The mismatch is visible in disassembly and reproduced at runtime:
group 4 returned signed `-10` (`0xFFFFFFF6`) despite a valid fourth profile.
This is a confirmed AS3000 defect; corresponding Neo runtime behavior remains
unqualified.

Vendor call-site inspection found an active-profile query followed by forwarding
its low byte to `FileCreate` in AlphaWord Plus (AS3000/Neo), Beamer
(AS3000/Neo), KeyWords (AS3000/Neo), KeyWords Wireless, AlphaQuiz, and Inspiration
Outliner. This is static caller evidence, not acceptance testing of all those
applets. The byte-return ABI matters: raw D0's upper bits need not be zero.

## Executed AS3000 qualification

The fixture cold-boots stock AlphaWord Plus and KeyWords in newly generated,
disposable flash/RAM. Only AlphaWord was exercised as a vendor UI consumer.
No physical device or saved user session was used. Assertions read controller
DDRAM and file/profile RAM; no screenshot or model inference was an oracle.

The successful run and full captured probe are preserved in
[`evidence/as3000-3.01`](evidence/as3000-3.01). The
[manifest](evidence/manifest.json) records input ROM/applet hashes and raw and
compressed capture hashes.

| Test | Observed result |
|---|---|
| Default state | Mode disabled. |
| Enable with native master-password dialog | Flag changed to enabled; firmware restarted applet control and entered selection. |
| Create Alice, Bobby, Carol, David | All four names/password records created; selected values 1, 2, 3, 4. |
| Existing group-zero AlphaWord text | Alice reopened the pre-enable `BASELINE` text. Saving renamed that same group-zero record; it did not migrate ownership. |
| Native AlphaWord Ctrl-N under Alice | Created group-1 `File 1` (`0x0101`); native keyboard editing stored/displayed `PRIVATEALICE`. |
| Grouped file creation | Groups 1–3 accepted the same `Shared` filename. |
| Duplicate check | Group 2 rejected `shared` after `Shared` with `-15`; spelling was retained. |
| Data and tag APIs | Group-2 write succeeded; matching-profile tag lookup succeeded; another profile's tag lookup returned `-11`. |
| Fourth profile file creation | Group 4 failed with `-10`. |
| Explicit cross-profile file ID | Read group-2 `BOB CONTENT` while profile 4 was selected. There is no ID-level ownership enforcement. |
| Return to Alice | Wrong password rejected; correct password selected Alice and AlphaWord reopened her native text. |
| Switch to Bobby | Correct Bobby password selected profile 2, but AlphaWord reopened Alice's `0x0101` document and displayed `PRIVATEALICE`. |
| Disable | Mode flag cleared; stored names remained; active-profile query's low byte returned zero. |

The cross-profile result has independent RAM evidence: in both
`alice-returned.ram.gz` and `bobby-sees-alice-text.ram.gz`, AlphaWord's cached file
pointer is `0x150A` and that record's file ID is `0x0101`; the latter capture's
selected profile byte is `0x02`. This supports a saved/current-file selection
problem, rather than merely similar text on two screens. The exact applet or
firmware change needed to repair it was not determined.

The file-selector checkpoint can occur while AlphaWord is still populating its
list. It is retained as a diagnostic only; the run makes **no complete directory
listing or file-selector isolation claim**.

Profile retention was tested across the native applet-control restart, **not**
a hardware reset, battery removal, power cycle, or subsequent emulator process.
Disabling was observed at flag transition and via the profile query; a complete
post-disable reboot and document workflow was not qualified. Cancellation,
all malformed names/passwords, allocator stress, every shortcut, all applets,
and physical hardware behavior remain outside coverage.

## Neo experiments are incomplete

An exploratory Neo 3.21 run reached initialized AlphaWord RAM and entered the
internal dialog control path, but its automation timed out waiting for an
expected ROM-string read. The trace hooks also observed prefetched/caller state;
those pointer samples cannot be interpreted as reliable function-entry argument
captures. See [failed automation log](evidence/neo-3.21-incomplete.txt).

That timeout is **not evidence that the Neo UI or filesystem failed**. No Neo
profile creation, fourth-group outcome, or stock-font screen was accepted.
Neo 3.01 received static inspection only. The incomplete Neo probe was not added
to the maintained reproduction runner. Future Neo display acceptance must use
a verified stock-font profile or independently authored pixel references under
the workspace's deterministic LCD rules; NeoFontTerminal cannot read these
firmware menus.

## Reproduction and follow-up

From the workspace root, with existing local stock ROMs/applets and MAME:

```sh
python3 betawise/research/multi-user/run.py /tmp/as3000-multi-user-new
python3 betawise/research/multi-user/verify-evidence.py
```

The output directory must not already exist. The probe intentionally invokes
private routines and characterizes known failures, and is research-only. A PASS
means its stated characterization assertions held, not that multi-user mode is
safe to expose. It preserves guest registers and its call trampoline/stack on
returning test calls; non-returning firmware restarts intentionally abandon the
saved call context. The rest of the disposable RAM contains test buffers and
synthetic profile data.

Before reconsidering exposure: repair/restrict the fourth-slot mismatch,
qualify saved/current-file selection across profiles and applets, define handling
of existing group-zero files, establish a supported switching lifecycle, and
test persistence and Neo variants. Preserve group metadata in backup formats,
but do not add privileged-root terminology or promise isolation in ordinary
paths based on this unfinished feature.
