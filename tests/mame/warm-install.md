# AS3000 Manager finalizer and warm applet registration

These addresses are for the AS3000 System 3.01 full ROM
`os3krom.os3kos`, mapped at `0x004c0000` in MAME. Its SHA-256 is
`732dca63399d883071ce41cc07ad352fa19c506b7b8c307ace16d5c53e6f8708`.

## Ghidra addresses

| Runtime address | File offset | Role |
| --- | --- | --- |
| **`0x004c1564`** | **`0x1564`** | **Manager selector `0x07`, finalize applet write.** Starts with `clr.w (0x3568).l`. |
| `0x004c0c3a` | `0x0c3a` | Manager request handler. The word-offset jump table at `0x004c0d24` has entry `0x0840` for selector `0x07`. Define the table in Ghidra to resolve this computed reference. |
| `0x004c1782` | `0x1782` | New-slot path in the finalizer writes the applet header pointer, flags, ID, and header metadata into the directory. The finalizer reduces the applet RAM allocation floor at `0x004c1598`. |
| `0x004c1870` | `0x1870` | Manager selector `0x08`, restart. Responds with `0x52`. The receive path calls the warm startup entry at `0x004c007a` after sending this response. |
| `0x004c78b0` | `0x78b0` | Pushes `WM_INIT` (`0x18`) before calling the registered applet at `0x004c78b6`. |
| `0x004d12d4` | `0x112d4` | A-line exception handler. Masks the opcode with `0x03fc` and uses it as a byte offset into the pointer table at `0x004eac1c`. |

The finalizer uses state created by the preceding install commands, including
the staged applet header pointer at guest RAM `0x0452`. It is a branch of the
Manager packet handler. None of the 256 pointers reachable through the
AS3000 A-line handler directly targets the packet handler, finalizer, restart
selector, or warm startup entry. This check leaves open indirect call chains.

## What the existing clients do

Local neotools sends the begin request, streams and programs blocks, then
sends `0x07` and waits for `0x48`. For a device initially in HID mode, the
cleanup path sends `0x08` to return it to keyboard mode. The applet install
path contains no file download and reupload sequence. The public
[Manager transaction analysis](https://github.com/SamDelorean/betawise/blob/os3k/base-analysis/docs/os3k-manager-applet-installation-transaction.md)
identifies the transaction in AS3000, NEO 2005, and NEO 2013 firmware. The
file-preserving AS3000 transport path remains to be tested end to end in MAME.

The branch review found no public syscall that registers an appended applet
and reinitializes the directory. A244 (`AppletSendMessage`) requires a
registered target and turns a forwarded `WM_INIT` into a synthetic message.
OS3K's applet startup checks for the plain `WM_INIT` value before initializing
its data. A244 is therefore unsuitable for registering the new image or
replaying its normal startup. See the
[A244 firmware analysis](https://github.com/SamDelorean/betawise/blob/os3k/base-analysis/docs/applet-send-message-a244-source-first-closure.md).

## MAME observations

Writing an image to flash through the SmartApplet slot preserved the AlphaWord
file, but warm startup kept using the old directory. A disposable snapshot
proved that registering a small fileless applet before asking the firmware to
reinitialize could preserve the file and deliver `WM_INIT` to both applets.
Clearing the full-initialization marker at `0x3566` erased the file. The
RAM-edit experiment has no implementation in the driver.

Driving the native Manager transaction would let the guest manage its applet
directory and file allocator. It requires a Manager transport and the flash
commands used by the guest, which the cartridge loader currently bypasses.
