#ifndef _OS3K_LOCALIZATION_H_
#define _OS3K_LOCALIZATION_H_

/** @file os3k_localization.h
 * @brief ROM byte-character conversion and alternative keyboard layouts.
 *
 * These operations use firmware character bytes, not Unicode or a host C locale.
 * Key events are converted by _OS3K_TranslateKeyToChar() in os3k_keyboard.h;
 * matrix positions first pass through TranslateMatrixKey(). The layout choice,
 * and the character converter's pending accent, are shared system state.
 */

#include <stdbool.h>
#include <stdint.h>
#include "os3k_keyboard.h"

/** @brief Alternative key arrangements selected by ConfigureKeyboardLayout().
 *
 * Values are ROM settings, not the numbers displayed by the selection dialog.
 * The dialog's choices 1, 2, 3, 4 select QWERTY, Dvorak, right-hand, left-hand.
 */
typedef enum _KeyboardLayout_e {
    KEYBOARD_LAYOUT_DVORAK = 0, /**< Two-handed Dvorak arrangement. */
    KEYBOARD_LAYOUT_LEFT_HAND = 1, /**< Left-handed Dvorak arrangement. */
    KEYBOARD_LAYOUT_RIGHT_HAND = 2, /**< Right-handed Dvorak arrangement. */
    KEYBOARD_LAYOUT_QWERTY = 3 /**< Default arrangement. */
} KeyboardLayout_e;

/** @brief Convert a firmware text byte to its ROM lowercase counterpart.
 *
 * Converts ASCII A–Z and firmware-specific encoded pairs;
 * other bytes are returned unchanged. This uses the ROM byte encoding, not
 * Unicode or a host locale, and does not modify a string. Some encoded values
 * change again on a second conversion; do not assume idempotence.
 * @param character Input byte, including zero or bytes above 0x7F.
 * @return Converted byte; only D0.b is meaningful in the raw ROM result.
 */
uint8_t ToLowerTextByte(uint8_t character);

/** @brief Translate a packed keyboard matrix position through the active layout.
 *
 * The high nibble is the matrix row (bit index), the low nibble
 * is its column. This is the same packing used by IsMatrixKeyDown(), not a
 * Key_e value or a PC scan code. Input 0x22 identifies the physical L key on
 * AS3000 3.01; QWERTY returns KEY_L. The layout changes key codes before character
 * conversion, not the interpretation of an already translated Key_e.
 *
 * Values 0x80–0xFF return KEY_NONE. For positions below 0x80 with no physical
 * key, QWERTY returns KEY_NONE, but alternative layouts index past their key
 * map with 0xFF: their results for those holes are not valid key contracts.
 * Call only for actual matrix positions when using an alternative layout.
 * @param matrix_position Packed row/column byte.
 * @return Translated Key_e byte. No modifier or release bits are added.
 */
Key_e TranslateMatrixKey(uint8_t matrix_position);

/** @brief Test membership in the ROM's fixed byte-character class.
 *
 * This opaque classification is independent of the selected
 * keyboard layout and font. Do not use it as isprint(), isalpha(), a Unicode
 * validator, or a test of whether the active display font contains a glyph.
 * @param character Byte to classify.
 * @return Canonical Boolean membership result.
 */
bool IsLocalizationTextByte(uint8_t character);

/** @brief Query the layout and optionally run its selection dialog.
 *
 * With prompt false, this only reads the setting and formats
 * optional output. With prompt true, clears the display and drains queued keys
 * before waiting. Keys 1–4 select an arrangement and require an acknowledgement
 * key; Escape or any other initial key leaves the arrangement unchanged.
 * The prompt does not restore the prior screen. Changing the layout here does
 * not reset the character converter's pending accent state.
 *
 * Both output pointers are optional. The description is overwritten with the
 * ROM's localized "Layout:" label, a space, and the short arrangement name.
 * With English firmware resources the longest result is "Layout: QWERTY" (14 bytes
 * including NUL). The ROM has no capacity parameter: supply ample storage for
 * the installed language resources, not merely the English length.
 * @param prompt True to display the interactive selector, false for query only.
 * @param layout Optional one-byte KeyboardLayout_e output (requires -fshort-enums).
 * @param description Optional writable character buffer; NULL skips formatting.
 * @return Always 1, including cancellation.
 */
uint32_t ConfigureKeyboardLayout(bool prompt, KeyboardLayout_e* layout, char* description);

#endif
