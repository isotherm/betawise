#ifndef _OS3K_H_
#define _OS3K_H_

/** @file os3k.h
 * @brief Public OS3K interfaces and selector-based firmware services.
 * Includes the subsystem headers; applications normally include this header.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "version.h"

#include "os3k_applet.h"
#include "os3k_dialog.h"
#include "os3k_display.h"
#include "os3k_file.h"
#include "os3k_irda.h"
#include "os3k_keyboard.h"
#include "os3k_localization.h"
#include "os3k_markup.h"
#include "os3k_power.h"
#include "os3k_serial.h"
#include "os3k_speech.h"
#include "os3k_wireless.h"
#include "os3k_stdlib.h"
#include "os3k_time.h"
#include "os3k_text.h"

/** @brief Four-byte firmware version returned by SYS_INT_GET_FIRMWARE_VERSION.
 *
 * This is a byte structure, not a host-endian integer. Neo copies the version
 * bytes from the system applet header and supplies a leading zero byte.
 */
typedef struct _FirmwareVersion_t {
    uint8_t reserved_zero; /**< Zero prefix byte. */
    uint8_t major; /**< System applet header's major version byte. */
    uint8_t minor; /**< System applet header's minor version byte. */
    char revision; /**< System applet header's revision character. */
} FirmwareVersion_t;

/** @brief Selector and data contracts for CallSysInt().
 *
 * AS3000 3.01 supports 0x0000, 0x0300, 0x0400, 0x0500, and 0x0600.
 * Other entries require compatible Neo firmware; the selector descriptions
 * below specify the Neo 3.21 interface. Scalar data occupies uint32_t
 * storage, including Boolean and byte-valued settings. Pointer queries write
 * into caller-provided pointer storage. Buffers have the stated fixed sizes.
 */
typedef enum _SysInt_e {
    SYS_INT_GET_HW_LEVEL = 0x0000, /**< uint32_t* output: 1 on AS3000 3.01, 2 on Neo. This does not distinguish Neo 2. */
    SYS_INT_GET_CONTRAST = 0x0100, /**< uint32_t* output: last applied contrast request byte, excluding the calibration bias. Neo only. */
    SYS_INT_SET_CONTRAST = 0x0200, /**< const uint32_t* input: low byte is added to the stored calibration bias with byte wrap; applied only when the resulting byte is below 64. Neo only. */
    SYS_INT_GET_ROW_HEIGHT = 0x0300, /**< uint32_t* output: 1 on AS3000 3.01; ROM-selected font height in pixels on Neo. Betawise substitutes its own font height when initialized. */
    SYS_INT_GET_LCD_HEIGHT = 0x0400, /**< uint32_t* output: 4 text rows on AS3000 3.01, firmware constant 65 on Neo. Neo raster bounds permit 66 pixel rows; this query reports 65. */
    SYS_INT_GET_LCD_WIDTH = 0x0500, /**< uint32_t* output: 40 text columns on AS3000 3.01, 264 pixels on Neo. */
    SYS_INT_GET_ROW_COUNT = 0x0600, /**< uint32_t* output: 4 on AS3000 3.01; floor(66 / ROM-selected font height) on Neo. Betawise substitutes its configured row count when initialized. */
    SYS_INT_GET_SYSTEM_FONT_PTR = 0x0700, /**< FontHeader_t** output: ROM system font pointer. Neo only. */
    SYS_INT_GET_ROM_SIZE = 0x0800, /**< uint32_t* output: flash chip-select size, 0x200000 or 0x400000 for recognized encodings, otherwise zero. Neo only. */
    SYS_INT_HAS_WIRELESS = 0x0900, /**< uint32_t* output: 1 when the ROM's wireless-detection byte is nonzero, otherwise 0. Neo only; detection is distinct from hardware level. */
    SYS_INT_WIRELESS_DISPLAY_STATUS = 0x0A00, /**< data unused. Draw firmware, wireless, network, MAC, and channel diagnostics. Performs radio queries and can wait for transport. */
    SYS_INT_WIRELESS_TURN_ON = 0x0B00, /**< data unused. Restart the asynchronous wireless connection flow: clear its operation state and set request bit 4. The worker can disassociate an existing network before proceeding. This is not a direct radio-power write. Neo only. */
    SYS_INT_SET_WIRELESS_BLOCK = 0x0C00, /**< const uint8_t[13] input: copy into the ROM block, then attempt wireless command 7 up to 1000 times. The block is opaque; a handled selector does not indicate transport completion. Neo only. */
    SYS_INT_0D00 = 0x0D00, /**< const uint32_t* input: store its low byte in a shared ROM flag. The flag is opaque firmware state. Paired with 0x0E00; Neo only. */
    SYS_INT_0E00 = 0x0E00, /**< uint32_t* output: zero-extended flag written by 0x0D00. The flag is opaque firmware state. Neo only. */
    SYS_INT_GET_WIRELESS_BLOCK = 0x0F00, /**< uint8_t[13] output: copy the opaque ROM block used by 0x0C00. Exactly 13 opaque bytes. Neo only. */
    SYS_INT_GET_FIRMWARE_VERSION = 0x1000, /**< FirmwareVersion_t* output: four version bytes. Requires a valid system applet descriptor. Neo only. */
    SYS_INT_GET_WIRELESS_NETWORK_NAME = 0x2000, /**< char[25] output: copy exactly 25 network-name bytes. No additional NUL is appended. Neo only. */
    SYS_INT_GET_SCRATCH_BUFFER_PTR = 0x3000, /**< void** output: shared, ROM-owned scratch arena. Query its capacity with SYS_INT_GET_SCRATCH_BUFFER_SIZE. This query does not allocate or grant exclusive ownership; do not free the pointer. Neo only. */
    SYS_INT_GET_SMALL_FONT_PTR = 0x4000, /**< FontHeader_t** output: ROM small-font pointer. Neo only. */
    SYS_INT_GET_SCRATCH_BUFFER_SIZE = 0x5000, /**< uint32_t* output: scratch arena capacity in bytes (8192 on Neo 3.21 firmware). Neo only. */
    /** uint32_t* output: 1 on Neo 3.21 firmware. Unsupported on AS3000 3.01
     * and Neo 3.01 firmware; unsupported queries leave data unchanged.
     * This selector does not enumerate individual extended syscall slots.
     */
    SYS_INT_8000 = 0x8000,
    SYS_INT_9000 = 0x9000, /**< uint32_t* output: opaque firmware predicate, returning 0 or 1. No public policy is assigned to this selector. Neo only. */
    SYS_INT_INIT_CONTRAST_BIAS = 0xA000 /**< const uint32_t* input: low byte initializes the flash contrast bias with marker 'Y' only if both stored bytes are erased. Does not apply contrast immediately. Neo only. */
} SysInt_e;

/** @brief Compare strings using an optional byte-sized limit and case folding.
 * Both zero and 255 select comparison through the first NUL; limits 1–254
 * compare at most that many bytes. Case folding uses the firmware byte encoding.
 * @param str1 First NUL-terminated string.
 * @param str2 Second NUL-terminated string.
 * @param case_sensitive Nonzero compares original bytes; zero folds both bytes.
 * @param num Byte limit, or zero/255 for an unbounded string comparison.
 * @return Eight-bit difference between the first unequal bytes, after folding
 * when requested. Subtraction wraps; its sign is not a general unsigned-byte
 * ordering guarantee.
 */
int8_t StringCompare(const char* str1, const char* str2, uint8_t case_sensitive, uint8_t num);

/** @brief Query or control ROM system services through selector-specific data.
 *
 * The ROM reads the low 16 bits of info and requires an
 * exact selector; unsupported selectors leave data untouched and return false.
 * A true result means the selector was handled, not that an underlying radio,
 * flash, or display operation succeeded. Scalar setters read the low byte of
 * a 32-bit input value; do not pass a pointer to just one byte.
 *
 * Betawise intercepts row-height and row-count queries when its own Neo font
 * is initialized. All other selectors go to the ROM. The ROM output pointers
 * are unchecked, so callers must supply valid storage of the documented size.
 * @param unused_zero Reserved argument; ignored by this interface. Pass zero.
 * @param info Service selector from SysInt_e.
 * @param data Selector-specific input, output, or buffer. May be NULL only
 * for selectors that explicitly do not use data, or for unsupported selectors.
 * @return True if supported and handled; false if unsupported. The native
 * result is a Boolean byte; upper return-register bits are not a status code.
 */
bool CallSysInt(uint32_t unused_zero, SysInt_e info, void* data);

/** @brief Copy the saved firmware exception record without clearing it.
 * All outputs are required. For bus/address errors, address is the fault address
 * and pc is the saved program counter.
 * @param marker Output for the firmware exception marker.
 * @param address Output for the saved fault address.
 * @param pc Output for the saved program counter.
 */
void GetExceptionInfo(uint32_t* marker, uint32_t* address, uint32_t* pc);

#endif
