#ifndef _OS3K_KEYBOARD_H_
#define _OS3K_KEYBOARD_H_

/** @file os3k_keyboard.h
 * @brief Key codes, modifiers, and keyboard routines.
 */

#include <stdbool.h>
#include <stdint.h>

/** @brief Firmware physical key codes. */
typedef enum _Key_e {
    KEY_FILE_1      = 0x2D,
    KEY_FILE_2      = 0x2C,
    KEY_FILE_3      = 0x04,
    KEY_FILE_4      = 0x0F,
    KEY_FILE_5      = 0x0E,
    KEY_FILE_6      = 0x0A,
    KEY_FILE_7      = 0x01,
    KEY_FILE_8      = 0x27,
    KEY_PRINT       = 0x2B,
    KEY_SPELL_CHECK = 0x35,
    KEY_FIND        = 0x3F,
    KEY_CLEAR_FILE  = 0x34,
    KEY_HOME        = 0x1F,
    KEY_END         = 0x3E,
    KEY_APPLETS     = 0x29,
    KEY_SEND        = 0x2A,
    KEY_GRAVE       = 0x2E,
    KEY_1           = 0x38,
    KEY_2           = 0x37,
    KEY_3           = 0x36,
    KEY_4           = 0x39,
    KEY_5           = 0x2F,
    KEY_6           = 0x30,
    KEY_7           = 0x3A,
    KEY_8           = 0x31,
    KEY_9           = 0x32,
    KEY_0           = 0x33,
    KEY_MINUS       = 0x28,
    KEY_EQUAL       = 0x25,
    KEY_BACKSPACE   = 0x03,
    KEY_TAB         = 0x06,
    KEY_Q           = 0x22,
    KEY_W           = 0x21,
    KEY_E           = 0x20,
    KEY_R           = 0x23,
    KEY_T           = 0x07,
    KEY_Y           = 0x09,
    KEY_U           = 0x24,
    KEY_I           = 0x1C,
    KEY_O           = 0x1D,
    KEY_P           = 0x1E,
    KEY_LEFTBRACE   = 0x02,
    KEY_RIGHTBRACE  = 0x00,
    KEY_BACKSLASH   = 0x15,
    KEY_CAPSLOCK    = 0x05,
    KEY_A           = 0x18,
    KEY_S           = 0x17,
    KEY_D           = 0x16,
    KEY_F           = 0x19,
    KEY_G           = 0x10,
    KEY_H           = 0x11,
    KEY_J           = 0x1B,
    KEY_K           = 0x12,
    KEY_L           = 0x13,
    KEY_SEMICOLON   = 0x14,
    KEY_APOSTROPHE  = 0x0B,
    KEY_ENTER       = 0x40,
    KEY_LEFTSHIFT   = 0x08,
    KEY_Z           = 0x43,
    KEY_X           = 0x42,
    KEY_C           = 0x41,
    KEY_V           = 0x44,
    KEY_B           = 0x4E,
    KEY_N           = 0x4F,
    KEY_M           = 0x46,
    KEY_COMMA       = 0x3B,
    KEY_DOT         = 0x3D,
    KEY_SLASH       = 0x47,
    KEY_RIGHTSHIFT  = 0x45,
    KEY_CTRL        = 0x4D,
    KEY_ALT         = 0x26,
    KEY_CMD         = 0x0C,
    KEY_SPACE       = 0x4C,
    KEY_ESC         = 0x48,
    KEY_DELETE      = 0x50,
    KEY_UP          = 0x4B,
    KEY_LEFT        = 0x49,
    KEY_DOWN        = 0x0D,
    KEY_RIGHT       = 0x4A,
    KEY_NONE        = 0xFF,
} Key_e;

/** @brief Modifier and release bits combined with a key code in an event word. */
typedef enum _KeyMod_e {
    KEY_MOD_CTRL        = 0x8000,
    KEY_MOD_CMD         = 0x4000,
    KEY_MOD_ALT         = 0x1000,
    KEY_MOD_RIGHTSHIFT  = 0x0800,
    KEY_MOD_LEFTSHIFT   = 0x0400,
    KEY_MOD_CAPS_LOCK   = 0x0200,
    KEY_MOD_KEY_UP      = 0x0080,
    KEY_MOD_NONE        = 0x0000,
    KEY_MOD_SHIFT       = (KEY_MOD_LEFTSHIFT | KEY_MOD_RIGHTSHIFT)
} KeyMod_e;

/** @brief Drain pending events and wait for a fresh keyboard event.
 *
 * The ROM polls the keyboard and processes special
 * keys while waiting.
 * @return A key code combined with event modifier bits.
 */
KeyMod_e WaitForKey(void);

/** @brief Remove the oldest event from the keyboard queue.
 * @param process_special_keys Enable ROM handling of special keys.
 * @return The event word, or KEY_NONE when the queue is empty.
 */
KeyMod_e GetKey(bool process_special_keys);

/** @brief Drain queued keyboard events with ROM special-key handling enabled.
 */
void DrainKeyBuffer(void);

/** @brief Check whether the keyboard queue contains an event.
 * @return True when an event is queued.
 */
bool IsKeyReady(void);

/** @brief Read the current keyboard modifier word.
 * @return The full 16-bit mask in KeyMod_e bit positions.
 */
uint16_t GetKeyModifiers(void);

/** @brief Run the ROM keyboard polling and event-processing routine.
 */
void ScanKeyboard(void);

/** @brief Append an event to the keyboard queue.
 *
 * A full queue silently drops the event.
 * @param key Key code and modifier bits combined into an event word.
 */
void QueueKey(KeyMod_e key);

/** @brief Replace the current keyboard modifier word.
 * @param mask Full 16-bit mask in KeyMod_e bit positions.
 */
void SetKeyModifiers(uint16_t mask);

/** @brief Sample the keyboard matrix for a held key.
 *
 * This reads the hardware independently of the queue.
 * @return True if the matrix reports a pressed key.
 */
bool IsKeyDownNow(void);

/** @brief Read a hardware keyboard matrix position directly.
 *
 * Selects a matrix column, reads the active-low
 * row inputs, and tests one row. It neither consumes queued events nor uses
 * the scanner's cached matrix, and it leaves the hardware column selection
 * changed. Matrix positions are distinct from Key_e codes. For example,
 * AS3000 3.01 L is matrix position 0x22 but key code KEY_L (0x13).
 * @param matrix_position Packed position: low nibble is the column selector
 * (0 through 15), high nibble is the row bit index (0 through 7). Selector
 * 8 samples the combined columns rather than isolating one column. Row
 * indices 8 through 15 cannot match the eight input bits and return false.
 * @return True when the selected input is asserted (the key is held).
 */
bool IsMatrixKeyDown(uint8_t matrix_position);

/** @brief Read whether automatic repetition of held keys is enabled.
 *
 * Reports the active Auto Repeat setting. Repeated
 * events are generated by ScanKeyboard(), subject to its repeat timing and
 * its rules for which keys can repeat.
 * @return True when Auto Repeat is enabled.
 */
bool IsAutoRepeatEnabled(void);

/** @brief Read whether modifier keys latch for sequential key combinations.
 *
 * With Sticky Keys enabled, a modifier can be
 * pressed and released before the key it modifies. An ordinary key consumes
 * the latched modifier; modifier and navigation keys have special handling.
 * @return True when Sticky Keys is enabled.
 */
bool IsStickyKeysEnabled(void);

/** @brief Enable or disable Sticky Keys.
 *
 * Updates the active flag and its RAM copy used
 * during keyboard reinitialization. This call does not clear the current
 * modifier word or itself write the setting to flash.
 * @param enabled True to latch modifiers for sequential combinations, false
 * to use physically held modifiers during normal scanning.
 */
void SetStickyKeysEnabled(bool enabled);

/** @brief Read whether keys must be held for an acceptance delay.
 *
 * When Slow Keys is enabled, a brief press is
 * rejected; a key held through the configured delay produces an event.
 * @return True when Slow Keys is enabled.
 */
bool IsSlowKeysEnabled(void);

/** @brief Read the configured Slow Keys acceptance-delay count.
 *
 * The scanner multiplies this count by 80 to obtain
 * a delay in milliseconds. It is not itself a millisecond or centisecond
 * value. The setting can be read even when Slow Keys is disabled. A value
 * of zero adds no acceptance delay; IsSlowKeysEnabled() reports the separate
 * enable flag. Actual event delivery also depends on keyboard polling.
 * @return Unsigned 16-bit count of 80 ms units. Multiply by 80 using 32-bit
 * arithmetic to obtain milliseconds without overflowing a 16-bit value.
 */
uint16_t GetSlowKeyDelay(void);

/** @brief Read whether keyboard scanning queues release events.
 * @return True when release-event generation is enabled.
 */
bool GetKeyReleaseEvents(void);

/** @brief Enable or disable release events from keyboard scanning.
 *
 * When enabled, releasing a key queues its event
 * with KEY_MOD_KEY_UP set. Press events are generated in either mode.
 * @param enabled True to queue key releases, false to queue only presses.
 */
void SetKeyReleaseEvents(bool enabled);

/** @brief Convert a translated key event to a firmware character byte.
 *
 * The low byte is a Key_e value, not a raw matrix
 * position; TranslateMatrixKey() applies the selected arrangement first.
 * Modifier groups select entries in the ROM key tables. Caps Lock uppercases
 * the unshifted result; Caps Lock plus Shift still produces the shifted entry,
 * rather than reversing case. Release events and key codes above 0x50 return 0.
 *
 * Conversion is stateful: Alt+E, Alt+Grave, Alt+I, Alt+U and Alt+N start acute,
 * grave, circumflex, diaeresis and tilde composition. A first accent returns 0;
 * a supported next letter returns its composed byte. A second accent emits the
 * previous accent. An unsupported next key emits the accent and requeues that
 * key for later processing. This shared state can affect subsequent callers.
 * Neo 3.21 firmware skips composition when no applet context is selected or the
 * selected header ID is 0xA002; its ordinary key-table conversion still runs.
 *
 * @param key Translated key code and modifier bits combined into an event word.
 * @return Firmware character byte, or zero for no immediate character (including
 * a pending accent). Only D0.b is meaningful. Cast to uint8_t before widening
 * bytes above 0x7F when plain char is signed; this is not a Unicode code point.
 */
char _OS3K_TranslateKeyToChar(KeyMod_e key);

/** @brief Convert a key event using the Betawise keyboard wrapper.
 *
 * The wrapper supplies Ctrl+C and Backspace when the ROM returns zero, and
 * dispatches the Applets key to the firmware menu.
 * @param key Key code and modifier bits combined into an event word.
 * @return Translated character, or zero if no character maps to the event.
 */
char TranslateKeyToChar(KeyMod_e key);

/** @brief Handler mask for SystemHandleEvents(); flags may be combined.
 *
 * The first three groups can enter native transfer/UI workflows. Select only
 * handlers appropriate to the applet's current interaction and transport state.
 */
typedef enum _SystemEventFlags_e {
    SYSTEM_EVENT_HANDLER_1 = 1, /**< Native cable/send workflow handler. */
    SYSTEM_EVENT_HANDLER_2 = 2, /**< Native infrared/transfer workflow handler. */
    SYSTEM_EVENT_HANDLER_4 = 4, /**< Pending external-link event dispatcher. */
    SYSTEM_EVENT_SPECIAL_KEYS = 8 /**< Dispatch the supplied special-key event. */
} SystemEventFlags_e;

/** @brief Run selected ROM system-event handlers.
 *
 * Handling an event can transfer control to the
 * firmware menu.
 * @param mask Bit 0x08 enables the special-key handler. Bits 0x01, 0x02,
 * and 0x04 select other system-event handlers.
 * @param key Event passed to the special-key handler when enabled.
 * @return Zero with special-key handling disabled, 8 when that handler
 * reports success, or -9 when it reports failure and control returns.
 */
int32_t SystemHandleEvents(SystemEventFlags_e mask, KeyMod_e key);

/** @brief Speed of text sent by emulating a host keyboard.
 *
 * These are the values in the ROM's Send-speed dialog, not baud rates.
 * The setter changes the active setting without changing the saved user
 * selection, except for the explicitly documented Neo-only special request.
 */
typedef enum _SendSpeed_e {
    SEND_SPEED_VERY_SLOW = 1, /**< Dialog choice 1: VERY Slow. */
    SEND_SPEED_SLOW = 2, /**< Dialog choice 2: Slow. */
    SEND_SPEED_FAST = 3, /**< Dialog choice 3: Fast; the initialized default. */
    SEND_SPEED_FASTEST = 4, /**< Dialog choice 4: Fastest. */
    SEND_SPEED_FASTEST_AND_DEFAULT = 0xDE /**< Neo 3.21 only: set both saved and active values to Fastest. Getter returns 4, not 0xDE. AS3000 3.01 rejects this request. */
} SendSpeed_e;

/** @brief Read the active keyboard-emulated Send speed.
 *
 * This setting is consumed by text-transfer code and is separate
 * from UART baud configuration and the keyboard's input repeat rate.
 * @return Active SendSpeed_e value, normally 1 through 4; only D0.b is meaningful.
 */
SendSpeed_e GetSendSpeed(void);

/** @brief Temporarily select a keyboard-emulated Send speed.
 *
 * Values 1 through 4 change the active setting only. Neo 3.21
 * Neo firmware also accepts SEND_SPEED_FASTEST_AND_DEFAULT, replacing both active and
 * saved user settings with 4. This is a RAM setting operation, not a claim of
 * flash persistence. Invalid values leave both settings unchanged.
 * @param speed Requested SendSpeed_e value.
 * @return Full signed 32-bit status: 0 on success, -1 for an invalid request.
 */
int32_t SetSendSpeed(SendSpeed_e speed);

#endif
