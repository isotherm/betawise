#ifndef _OS3K_DIALOG_H_
#define _OS3K_DIALOG_H_

/** @file os3k_dialog.h
 * @brief ROM text input, modal messages, singleton lists, and Neo font-aware dialogs.
 *
 * DialogInit() resets the shared dialog builder. Item strings remain owned
 * by the caller and must stay valid through drawing and interaction.
 * FontDialog services describe the inspected Neo 3.21 ROM and are outside
 * the AS3000 3.01 and Neo 3.01 syscall tables.
 */

#include "os3k_keyboard.h"
#include <stddef.h>

/** @brief Edit a NUL-terminated text buffer at the current cursor.
 *
 * The buffer is echoed on entry. Backspace removes
 * a character and the length output is updated as text is entered.
 * @param buffer Writable storage of at least max_len + 1 bytes.
 * @param len Input/output byte length, initially no greater than max_len.
 * @param max_len Maximum text length in bytes, excluding the terminator.
 * @param exit_keys Byte-sized key codes terminated by KEY_NONE. Internal
 * values 0xFD and 0xFE alter special-key and system-event handling.
 * @param password Value true masks entered characters with asterisks.
 * @return Exit key code, or 0xFE for the enabled system-event exit path.
 */
uint8_t TextBox(char* buffer, uint8_t* len, uint8_t max_len,
                const Key_e* exit_keys, bool password);

/** @brief Clear the screen, show a message, and wait for a fresh key.
 * @param str NUL-terminated message.
 * @return The keyboard event used to dismiss the message.
 */
KeyMod_e DisplayMessage(const char* str);

/** @brief Reset the shared list-dialog builder.
 *
 * At most 64 items and 15 exit keys can be added.
 * @param single True requests a single column. False allows multiple columns.
 * @param row_first First display row, one-based.
 * @param row_last Last display row, inclusive.
 * @param width Available display width in character cells, starting at column 1.
 */
void DialogInit(bool single, uint8_t row_first, uint8_t row_last, uint8_t width);

/** @brief Append an item to the shared dialog.
 *
 * The ROM retains the text pointer.
 * @param text Item text with lifetime covering the dialog.
 * @param text_len Display length of the item text.
 * @param marker Item marker, usually a space.
 * @param id Caller-defined value retrieved by the item-ID queries.
 * @param shortcut_key Optional shortcut annotation. KEY_NONE suppresses it.
 * @param file_size Optional size annotation. Pass (size_t)-1 to suppress it.
 * @return Zero on success or -1 when the 64-item table is full.
 */
int DialogAddItem(const char* text, uint8_t text_len, char marker, int id,
                  Key_e shortcut_key, size_t file_size);

/** @brief Add a key that exits DialogRun().
 * @param key Exit key code.
 * @return Zero on success or -1 when the 15-key table is full.
 */
int DialogAddExitKey(Key_e key);

/** @brief Store the selected one-based item index.
 *
 * This setter does not validate the index.
 * @param index Item index from 1 through the number of items.
 */
void DialogSetChoice(uint8_t index);

/** @brief Lay out and draw the shared list dialog.
 */
void DialogDraw(void);

/** @brief Process keyboard input for the shared dialog.
 *
 * Navigation updates the one-based selection.
 * @return The exit keyboard event, or a ROM system-event result.
 */
uint16_t DialogRun(void);

/** @brief Get the selected one-based item index.
 * @return Stored selection index.
 */
uint8_t DialogGetChoice(void);

/** @brief Get the caller-defined ID of the selected item.
 *
 * Requires a valid selected index.
 * @return Selected item's ID.
 */
int DialogGetChoiceId(void);

/** @brief Get the caller-defined ID of an item.
 * @param index One-based item index.
 * @return Item ID, or zero when the index is outside the item table.
 */
int DialogGetItemId(uint8_t index);

/** @brief Get the retained text pointer and display length of a dialog item.
 *
 * This does not copy or allocate text. The caller that added
 * the item still owns the string. The index is unchecked: use an existing
 * one-based item index, not zero or an index beyond the current item count.
 * The Betawise wrapper adapts the ROM's D0 pointer result to GCC's A0 result.
 * @param index Existing one-based item index.
 * @param text_len Required output byte receiving the length passed to DialogAddItem().
 * @return The exact text pointer retained by DialogAddItem().
 */
const char* DialogGetItemText(uint8_t index, uint8_t* text_len);

/** @brief Display the native printer-selection dialog.
 *
 * Lists seven printer families. Enter commits the highlighted choice; Escape
 * preserves the previous choice. Some choices require the printer applet.
 * Changes the screen and uses the shared list-dialog storage.
 */
void DialogSelectPrinter(void);

/** @brief ROM-owned font-aware dialog; allocate with FontDialogCreate().
 *
 * Each object contains up to five regions. Region indices are zero-based and
 * unchecked by most services. Only one dialog and region are globally active.
 * The five input-editing records are shared across objects by region index;
 * initializing or destroying one object can replace another object's input state.
 */
typedef struct _FontDialog_t FontDialog_t;

/** @brief Built-in font-dialog templates; 8 and 9 share a template. */
typedef enum _FontDialogLayout_e {
    FONT_DIALOG_LAYOUT_5 = 5, /**< Five-region template, including two input fields. */
    FONT_DIALOG_LAYOUT_6 = 6, /**< Three-region template, including two input fields. */
    FONT_DIALOG_LAYOUT_7 = 7, /**< Single-region template with title, input and body. */
    FONT_DIALOG_LAYOUT_8 = 8, /**< Single-region template with line selection. */
    FONT_DIALOG_LAYOUT_9 = 9, /**< Same base geometry as template 8. */
    FONT_DIALOG_LAYOUT_11 = 11, /**< Title region and two body columns, split 40/60. */
    FONT_DIALOG_LAYOUT_12 = 12, /**< Single-region template with choice-style input. */
    FONT_DIALOG_LAYOUT_13 = 13, /**< Single-region title/body template. */
    FONT_DIALOG_LAYOUT_14 = 14, /**< Single-region template with underlined input. */
    FONT_DIALOG_LAYOUT_15 = 15, /**< Single-region template with editable input. */
    FONT_DIALOG_LAYOUT_16 = 16, /**< Main region plus two half-width lower regions. */
    FONT_DIALOG_LAYOUT_17 = 17, /**< Three-region variant with choice-style input. */
    FONT_DIALOG_LAYOUT_18 = 18, /**< Single-region variant based on template 8. */
    FONT_DIALOG_LAYOUT_19 = 19, /**< Title region and two equal body columns. */
    FONT_DIALOG_LAYOUT_20 = 20, /**< Title region and two equal title/body columns. */
    FONT_DIALOG_LAYOUT_21 = 21 /**< Title region, body column and input/body column. */
} FontDialogLayout_e;

/** @brief Property selectors, rather than bit masks, for FontDialogSetProperty(). */
typedef enum _FontDialogProperty_e {
    FONT_DIALOG_SHOW_TITLE = 0, /**< Region flag 0x80: draw the title row. */
    FONT_DIALOG_ENABLE_INPUT = 1, /**< Region flag 0x40: handle/draw input and its cursor. */
    FONT_DIALOG_UNDERLINE_INPUT = 2, /**< Region flag 0x20: underline populated input. */
    FONT_DIALOG_SHOW_BODY = 3, /**< Region flag 0x10: draw the body/list. */
    FONT_DIALOG_BODY_OVERFLOW = 4, /**< Region flag 0x08: body overflow; drawing recomputes it. */
    FONT_DIALOG_SELECT_ITEMS = 5, /**< Region flag 0x04: highlight the selected body item. */
    FONT_DIALOG_SELECT_LINE = 6, /**< Region flag 0x02: select one displayed line instead of an item span. */
    FONT_DIALOG_ALWAYS_UNDERLINE = 7 /**< Region flag 0x01: draw the input underline unconditionally. */
} FontDialogProperty_e;

/** @brief Native animation styles; apply only to a valid dialog region. */
typedef enum _FontDialogAnimation_e {
    FONT_DIALOG_ANIMATION_BLOCK_CURSOR = 1, /**< Blink a five-pixel block caret at the text position. */
    FONT_DIALOG_ANIMATION_THIN_CURSOR = 2, /**< Blink a two-pixel caret at the text position. */
    FONT_DIALOG_ANIMATION_3 = 3, /**< Native style 3; updates state but has no dedicated drawing branch. */
    FONT_DIALOG_ANIMATION_SIGNAL = 4, /**< Animate a twelve-pixel radio-signal bitmap. */
    FONT_DIALOG_ANIMATION_SCROLL_BOTH = 5, /**< Blink the combined up/down scroll-arrow bitmap. */
    FONT_DIALOG_ANIMATION_SCROLL_UP = 6, /**< Blink an upward scroll-arrow bitmap. */
    FONT_DIALOG_ANIMATION_SCROLL_DOWN = 7 /**< Blink a downward scroll-arrow bitmap. */
} FontDialogAnimation_e;

/** @brief Four text inputs supplied together to FontDialogSetContent(). */
typedef struct _FontDialogContent_t {
    const char* title; /**< Borrowed left title string, or NULL to leave it unchanged. */
    const char* rightTitle; /**< Borrowed right title string, or NULL to leave unchanged. */
    const char* body; /**< Borrowed body/item text, or NULL to leave unchanged. */
    const char* inputPrompt; /**< Copied protected input prompt/choice configuration, or NULL. */
} FontDialogContent_t;

/** @brief Inline image descriptor used by font-dialog text bytes 1–7. */
typedef struct _DialogBitmap_t {
    uint16_t width; /**< Bitmap width in pixels. */
    uint16_t height; /**< Bitmap height in pixels. */
    const uint8_t* pixels; /**< Borrowed packed bitmap storage. */
} DialogBitmap_t;

/** @brief Replace selected region text and redraw.
 *
 * NULL text arguments preserve existing fields. Input text is copied; title
 * and body pointers remain borrowed. Passing NULL as dialog performs no work.
 * @param dialog Existing dialog, or NULL.
 * @param region Existing zero-based region index.
 * @param title New left title, or NULL.
 * @param rightTitle New right title, or NULL.
 * @param body New body text, or NULL; resets body selection when supplied.
 * @param inputPrompt Protected input prompt/choice configuration, or NULL; keep it below eighty bytes.
 * @param indentPixels Pixel indentation used by caret markup in text.
 */
void FontDialogUpdate(FontDialog_t* dialog, int8_t region, const char* title,
                      const char* rightTitle, const char* body, const char* inputPrompt,
                      uint16_t indentPixels);

/** @brief Replace left title and redraw with 55-pixel indentation.
 * @param dialog Existing dialog.
 * @param region Existing zero-based region index.
 * @param title Borrowed replacement string, or NULL to preserve it.
 */
void FontDialogSetTitle(FontDialog_t* dialog, int8_t region, const char* title);

/** @brief Replace right title and redraw with 55-pixel indentation.
 * @param dialog Existing dialog.
 * @param region Existing zero-based region index.
 * @param title Borrowed replacement string, or NULL to preserve it.
 */
void FontDialogSetRightTitle(FontDialog_t* dialog, int8_t region, const char* title);

/** @brief Replace body text and redraw with 55-pixel indentation.
 * @param dialog Existing dialog.
 * @param region Existing zero-based region index.
 * @param text Borrowed body/item string, or NULL to preserve it.
 */
void FontDialogSetBody(FontDialog_t* dialog, int8_t region, const char* text);

/** @brief Initialize a copied input prompt/configuration and redraw.
 *
 * Ordinary input treats this string as a protected prefix; a trailing colon
 * gains a space. Subsequent typing is appended after that prefix. Choice-style
 * input interprets its own configuration markers.
 * @param dialog Existing dialog.
 * @param region Existing zero-based region index.
 * @param prompt Prompt/configuration shorter than eighty bytes; NULL only redraws.
 */
void FontDialogSetInput(FontDialog_t* dialog, int8_t region, const char* prompt);

/** @brief Configure the title animation state.
 *
 * Styles 5–7 attach to the right title; other styles attach to the left title.
 * @param dialog Existing dialog.
 * @param region Existing zero-based region index.
 * @param style Native animation style.
 * @param enabled True attaches animation to its drawing region; false detaches it.
 */
void FontDialogSetTitleAnimation(FontDialog_t* dialog, int8_t region,
                                 FontDialogAnimation_e style, bool enabled);

/** @brief Configure animation for a body row.
 * @param dialog Existing dialog.
 * @param region Existing zero-based region index.
 * @param style Native animation style.
 * @param enabled Whether to attach the animation.
 * @param row Body-row index from zero through five.
 */
void FontDialogSetRowAnimation(FontDialog_t* dialog, int8_t region,
                               FontDialogAnimation_e style, bool enabled, uint8_t row);

/** @brief Configure the input-region animation.
 * @param dialog Existing dialog.
 * @param region Existing zero-based region index.
 * @param style Native animation style.
 * @param enabled Whether to attach the animation.
 */
void FontDialogSetInputAnimation(FontDialog_t* dialog, int8_t region,
                                 FontDialogAnimation_e style, bool enabled);

/** @brief Set one region flag without drawing.
 * @param dialog Existing dialog.
 * @param region Existing zero-based region index.
 * @param property Selector from FontDialogProperty_e.
 * @param enabled New flag value.
 */
void FontDialogSetProperty(FontDialog_t* dialog, int8_t region,
                           FontDialogProperty_e property, bool enabled);

/** @brief Process one character/control word in the active font dialog.
 * @param key Character/control word. Printable values 0x20–0xFF edit input;
 * 0x0009 changes focus. The ROM also handles control-key words such as
 * 0x804B/0x824B, 0x800D/0x820D, 0x8049/0x8249, and 0x804A/0x824A.
 * Ordinary input characters must be translated from matrix key codes first.
 */
void FontDialogHandleKey(uint16_t key);

/** @brief Read the active region's zero-based body selection byte.
 * @return Current body item/line index; requires an active region.
 */
uint8_t FontDialogGetSelection(void);

/** @brief Store the active region's body selection without bounds checks.
 * @param selection Zero-based item/line index valid for the active region.
 */
void FontDialogSetSelection(uint8_t selection);

/** @brief Clear selected-item drawing attributes.
 * @param mask Attribute bits to clear across the active selected item's displayed rows.
 */
void FontDialogClearItemAttributes(uint8_t mask);

/** @brief Set drawing attributes on the selected displayed body row.
 * @param mask Attribute bits ORed into that row's drawing attributes.
 */
void FontDialogSetRowAttributes(uint8_t mask);

/** @brief Redraw one dialog region.
 * @param dialog Existing dialog.
 * @param region Existing zero-based region index.
 */
void FontDialogDrawRegion(FontDialog_t* dialog, int8_t region);

/** @brief Release a dialog's pool slot and clear shared input records.
 *
 * Does not clear the global active-dialog pointer. Stop using the dialog before release.
 * @param dialog Existing dialog, or NULL.
 */
void FontDialogDestroy(FontDialog_t* dialog);

/** @brief Allocate and initialize a built-in dialog.
 *
 * Uses a twenty-object ROM pool. The wrapper adapts the D0 pointer return to GCC A0.
 * @param layout Built-in template to instantiate.
 * @return Borrowed ROM-owned object, or NULL for invalid layout or pool exhaustion.
 */
FontDialog_t* FontDialogCreate(FontDialogLayout_e layout);

/** @brief Replace region content from four pointers and redraw.
 * @param dialog Existing dialog.
 * @param region Existing zero-based region index.
 * @param content Four text pointers; NULL members preserve existing content. Uses zero indentation.
 */
void FontDialogSetContent(FontDialog_t* dialog, int8_t region, const FontDialogContent_t* content);

/** @brief Select the global active dialog and region.
 *
 * Passing the already-active object leaves the selection unchanged. Passing
 * NULL directly selects a region of the current object without normal focus traversal.
 * @param dialog Object to activate, or NULL to reuse the current object.
 * @param region Requested zero-based region index.
 */
void FontDialogActivate(FontDialog_t* dialog, int8_t region);

/** @brief Copy input from the active/default region.
 *
 * The first argument is ignored by this ROM; selection uses global dialog state.
 * Choice-style input writes one byte without a terminator; other styles copy
 * the editable suffix and append NUL. Provision at least 82 output bytes.
 * @param ignored Retained native argument; pass NULL.
 * @param output Input-result storage.
 * @return Zero-based region from which the input was obtained, in the low byte.
 */
uint8_t FontDialogGetInput(const FontDialog_t* ignored, char* output);

/** @brief Lay out/count text using a region's first body-row geometry.
 * @param dialog Existing dialog.
 * @param region Existing zero-based region index.
 * @param text Text terminated by NUL or carriage return; the stored body pointer is unchanged.
 * @return Native wrapped-line count, in the low word.
 */
uint16_t FontDialogWrapText(FontDialog_t* dialog, int8_t region, const char* text);

/** @brief Advance/redraw animation states in the current dialog.
 * @param event Event byte; 0xFF suppresses updates. Association UI can also suppress them.
 */
void FontDialogUpdateAnimations(uint8_t event);

/** @brief Measure text with the current font, excluding slash markup.
 * @param text NUL-terminated text; slash escapes the following byte, while // contributes one slash.
 * @param length Maximum stored bytes to inspect.
 * @return Pixel width accumulated in sixteen bits.
 */
uint16_t FontDialogTextWidth(const char* text, uint16_t length);

/** @brief Register an inline bitmap for dialog text.
 * @param slot Registration slot 1–8; other values do nothing. Text rendering
 * uses custom slots 1–7, while byte 8 draws a built-in image.
 * @param bitmap Borrowed width/height/pixels descriptor; retain it while registered.
 * @param xOffset Horizontal pixel offset; only the low byte is retained.
 * @param yOffset Vertical pixel offset; only the low byte is retained.
 */
void FontDialogSetBitmap(uint8_t slot, const DialogBitmap_t* bitmap,
                         uint16_t xOffset, uint16_t yOffset);

#endif
