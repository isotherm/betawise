#ifndef _OS3K_TEXT_H_
#define _OS3K_TEXT_H_

/** @file os3k_text.h
 * @brief Native file-backed editor, encoded-text scanner, and forms.
 *
 * Contracts describe AS3000 3.01 and Neo 3.21 firmware.
 * Structures use the native m68k ABI, including two-byte alignment of 32-bit
 * fields. The ROM owns their internal state; initialize with these routines
 * rather than constructing partially populated records. File operations can
 * move backing storage, so retain file offsets rather than borrowed pointers.
 * The editor stores an internal gap in its file; raw file length includes it.
 */
#include <stdbool.h>
#include <stdint.h>
#include "os3k_applet.h"
#include "os3k_display.h"

/** @brief Native text-operation status constants.
 *
 * Status-returning operations return zero on success. They can also propagate
 * negative filesystem errors, so their return type is signed int32_t.
 */
#define TEXT_ERROR_NO_SPACE       INT32_C(0x01000001) /**< Required file growth failed. */
#define TEXT_ERROR_INVALID_TEXT   INT32_C(0x01000002) /**< Encoded text is invalid for the requested operation. */
#define TEXT_ERROR_RECTANGLE      INT32_C(0x02000001) /**< Requested viewport exceeds the screen. */
#define TEXT_ERROR_ORIGIN         INT32_C(0x02000002) /**< Invalid scan-position origin. */
#define TEXT_ERROR_UNIMPLEMENTED  INT32_C(0x02000003) /**< Operation is not implemented by this firmware. */
#define TEXT_ERROR_FILE           INT32_C(0x02000004) /**< File does not meet editor requirements or a file operation failed. */
#define TEXT_ERROR_READ_ONLY      INT32_C(0x02000005) /**< Operation requires an editable editor. */
#define TEXT_ERROR_CURRENT_FILE   INT32_C(0x02000006) /**< Editor's file is not current, or its ID is zero. */
#define TEXT_ERROR_INVALID_FORM   INT32_C(0x04000001) /**< Form syntax or structure is invalid. */

/** @brief Initialization flags for TextEditorInit(). */
typedef enum _TextEditorFlags_e {
    TEXT_EDITOR_EDITABLE = 0, /**< Open the file and initialize its edit/layout state. */
    TEXT_EDITOR_READ_ONLY = 1, /**< Resolve without selecting the file; prohibit scanner insert/delete. */
    TEXT_EDITOR_DEFER_LAYOUT = 2 /**< Populate the basic descriptor and return before text/layout initialization. */
} TextEditorFlags_e;

/** @brief Starting state for a text scan position. */
typedef enum _TextScanOrigin_e {
    TEXT_POSITION_START = 0, /**< Begin at raw offset zero with cleared scanner state. */
    TEXT_POSITION_VIEWPORT = 1, /**< Start at the editor's saved viewport offset/state. */
    TEXT_POSITION_CARET = 2 /**< Start at the editor's caret offset/state. */
} TextScanOrigin_e;

/** @brief Unit and conversion policy for text scanning and editing. */
typedef enum _TextByteMode_e {
    TEXT_BYTES_ENCODED = 0, /**< Preserve complete native encoded tokens; count stored bytes. */
    TEXT_BYTES_PLAIN = 1 /**< Convert native tokens to plain text; count emitted bytes. */
} TextByteMode_e;

/** @brief Native 94-byte editor descriptor, shared in size by AS3000 3.01 and Neo.
 *
 * Offsets and gap boundaries refer to the encoded file. Fields marked opaque
 * are managed by the ROM and must be preserved. Font pointers are Neo state;
 * the corresponding AS3000 3.01 storage is reserved.
 */
typedef struct _TextEditor_t {
    uint16_t file_id; /**< 0x00, resolved file identifier. */
    uint32_t max_length; /**< 0x02, maximum encoded content length; zero means no caller limit. */
    uint8_t internal_06[8]; /**< 0x06, opaque editing/layout state. */
    uint16_t left; /**< 0x0E, viewport left coordinate. */
    uint16_t top; /**< 0x10, viewport top coordinate. */
    uint16_t width; /**< 0x12, viewport width. */
    uint16_t layout_height; /**< 0x14, native vertical layout extent; rows on AS3000 3.01, pixels on Neo. */
    uint16_t height; /**< 0x16, viewport height, in text rows on AS3000 3.01 and pixels on Neo. */
    uint32_t viewport_offset; /**< 0x18, raw file offset at the viewport origin. */
    uint16_t caret_scan_flags; /**< 0x1C, scanner state at the caret; ROM-managed. */
    uint16_t caret_scan_line; /**< 0x1E, opaque scanner line state. */
    uint16_t viewport_scan_flags; /**< 0x20, scanner state at the viewport origin. */
    uint16_t viewport_scan_line; /**< 0x22, opaque viewport line state. */
    uint32_t caret_offset; /**< 0x24, caret's raw file offset. */
    uint32_t gap_start; /**< 0x28, first byte of the editor's internal gap. */
    uint32_t gap_end; /**< 0x2C, raw offset immediately after the internal gap. */
    uint16_t internal_30[2]; /**< 0x30, opaque layout state. */
    uint16_t selection_flags; /**< 0x34, scanner flags saved at the selection anchor. */
    uint16_t selection_state; /**< 0x36, zero without a selection; nonzero selection/drawing state. */
    uint32_t selection_anchor; /**< 0x38, selection anchor's raw file offset. */
    uint16_t caret_x; /**< 0x3C, caret's viewport-relative horizontal coordinate. */
    uint16_t caret_y; /**< 0x3E, caret's viewport-relative vertical coordinate. */
    uint16_t internal_40; /**< 0x40, opaque layout state. */
    uint32_t form_start; /**< 0x42, embedded form region start, excluding its enclosing marker. */
    uint32_t form_end; /**< 0x46, embedded form region end. Zero when absent. */
    uint16_t read_only; /**< 0x4A, nonzero for a read-only descriptor. */
    uint16_t cursor_style; /**< 0x4C, saved native cursor-style byte widened to a word. */
    FontHeader_t* fonts[4]; /**< 0x4E, four borrowed Neo font descriptors; reserved on AS3000 3.01. */
} TextEditor_t;

/** @brief Native 44-byte encoded-text traversal position.
 *
 * A position borrows its editor. Its offset is a raw file position, even when
 * scanning counts plain-text bytes. Scanning changes the position but does not
 * move the editor caret until TextEditorSetScanPosition() is called.
 */
typedef struct _TextScan_t {
    TextEditor_t* editor; /**< 0x00, borrowed editor descriptor. */
    uint32_t offset; /**< 0x04, next raw file offset. */
    uint16_t x; /**< 0x08, scanner horizontal layout state. */
    uint16_t y; /**< 0x0A, scanner vertical layout state. */
    uint16_t flags; /**< 0x0C, token flags; low bits 1/2 represent hidden/protected regions. */
    uint16_t line_state; /**< 0x0E, opaque scanner line state. */
    FontHeader_t* font; /**< 0x10, borrowed Neo scan font; reserved on AS3000 3.01. */
    uint8_t expression_state[18]; /**< 0x14, opaque form/expression conversion state. */
    const char* conversion_text; /**< 0x26, ROM-owned pending conversion text. */
    uint8_t conversion_phase; /**< 0x2A, pending conversion state-machine phase. */
    uint8_t reserved_2b; /**< 0x2B, padding/reserved; preserve it. */
} TextScan_t;

/** @brief Native 124-byte transfer state initialized for an editor or a form.
 *
 * Pass messages from the matching transport workflow to its handler. This is
 * shared firmware transfer state, not a UART buffer or a file allocation.
 */
typedef struct _TextTransfer_t {
    uint16_t file_id; /**< 0x00, source file identifier. */
    uint8_t phase; /**< 0x02, ROM transfer state-machine phase. */
    uint8_t flags; /**< 0x03, ROM transfer policy bits. */
    const uint8_t* ranges[6]; /**< 0x04, borrowed form/transfer range cursors; ROM-managed. */
    uint16_t key_state; /**< 0x1C, ROM transfer key/progress state. */
    uint16_t chunk_width; /**< 0x1E, ROM transfer chunk policy. */
    char name[40]; /**< 0x20, copied transfer name, including its NUL terminator. */
    uint32_t progress; /**< 0x48, ROM transfer progress state. */
    uint32_t text_length; /**< 0x4C, converted text length used by the transfer. */
    TextScan_t position; /**< 0x50, embedded editor scan position; unused fields remain ROM-managed. */
} TextTransfer_t;

/** @brief Native 74-byte parsed-form and form-view state.
 *
 * The parser can append FORMDATA and normalize newlines in the file. Offsets
 * identify the encoded form and generated data, not host HTML objects.
 */
typedef struct _TextForm_t {
    uint16_t file_id; /**< 0x00, selected form file. */
    uint32_t start; /**< 0x02, first raw byte of the form region. */
    uint32_t end; /**< 0x06, raw offset after the form and generated data. */
    uint32_t data_tag; /**< 0x0A, generated FORMDATA opening-tag offset. */
    uint32_t data_start; /**< 0x0E, start of generated form values. */
    uint32_t data_end; /**< 0x12, start of the closing FORMDATA tag. */
    FontHeader_t* font; /**< 0x16, borrowed Neo font; reserved on AS3000 3.01. */
    uint16_t top_row; /**< 0x1A, current form scroll row. */
    uint8_t internal_1c[10]; /**< 0x1C, opaque form layout state. */
    uint32_t active_tag; /**< 0x26, raw offset of the active control's tag text. */
    uint16_t control_count; /**< 0x2A, parsed control count. */
    int16_t active_control; /**< 0x2C, focused control index; -1 if there are no controls. */
    uint16_t control_flags; /**< 0x2E, current control's ROM flags. */
    uint8_t internal_30[26]; /**< 0x30, opaque active-control layout/edit state. */
} TextForm_t;

/** @brief Initialize a file-backed editor.
 *
 * AS3000 3.01 coordinates are zero-based columns/rows, within 40 by 4. Neo bounds
 * are 264 by 66 pixels: the legacy full-screen 0,0,40,4 expands to that size;
 * smaller legacy extents may be multiplied by 6 horizontally and 16 vertically.
 * Editable initialization can normalize tokens and create an internal gap.
 * Read-only/deferred initialization requires the file's flag 0x04 to be set.
 * Deferred layout leaves a partial descriptor, not a ready editable state.
 * @param editor Required writable descriptor.
 * @param file_id File selector; read-only mode resolves the low file number for the active user, or clipboard ID CB.
 * @param flags Combination of TextEditorFlags_e values.
 * @param max_length Maximum encoded content bytes, excluding the internal gap; zero disables this limit.
 * @param left Viewport left coordinate.
 * @param top Viewport top coordinate.
 * @param width Viewport width.
 * @param height Viewport height.
 * @return Zero, a TEXT_ERROR_* value, or a negative filesystem error.
 */
int32_t TextEditorInit(TextEditor_t* editor, uint16_t file_id, TextEditorFlags_e flags,
                       uint32_t max_length, uint16_t left, uint16_t top,
                       uint16_t width, uint16_t height);

/** @brief Handle an editor message.
 * @param editor Initialized editor; an editable editor's file must be current.
 * @param message Applet message, including idle, key, character, and font-change notifications.
 * @param param Message-specific scalar, such as a key event or character byte.
 * @param context Forwarded context value; pass zero without a native workflow requiring it.
 * @param status Required operation-status output, cleared before handling.
 * @return One when consumed, zero when unhandled, or TEXT_ERROR_CURRENT_FILE.
 */
int32_t TextEditorHandleMessage(TextEditor_t* editor, Message_e message, uint32_t param,
                                uint32_t context, int32_t* status);

/** @brief Reopen and reconcile an editor with its file.
 * @param editor Initialized descriptor. Can trigger layout or reinitialization.
 * @return Zero, a TEXT_ERROR_* value, or a negative filesystem error.
 */
int32_t TextEditorRefresh(TextEditor_t* editor);

/** @brief Initialize a scan position.
 * @param editor Initialized descriptor that outlives the position.
 * @param position Required writable position.
 * @param origin Start, viewport, or caret snapshot. Invalid values still partially reset the output.
 * @return Zero or TEXT_ERROR_ORIGIN.
 */
int32_t TextScanInit(TextEditor_t* editor, TextScan_t* position, TextScanOrigin_e origin);

/** @brief Scan, copy, or search encoded text.
 *
 * No NUL is appended. Encoded mode stops at token boundaries. Plain mode
 * strips formatting, maps text equivalents, and unwraps escaped characters.
 * Search forces plain conversion, moves position to a match, and returns a
 * Boolean result instead of a byte count. Keep editor/file storage valid.
 * @param position Required mutable scan state.
 * @param count Maximum encoded or emitted bytes; use UINT32_MAX to scan to a boundary.
 * @param output Optional buffer of at least count bytes; NULL skips copying.
 * @param reverse True to scan backward. Output is emitted in reverse traversal order.
 * @param mode Encoded-byte or plain-text counting/copying.
 * @param skip_hidden Skip tokens in region flag 0x01.
 * @param skip_protected Skip tokens in region flag 0x02.
 * @param expand_forms Append converted embedded-form text after the ordinary forward scan, wrapped in FORM tags. Ignored for backward conversion/search.
 * @param case_sensitive True for exact-byte searching; false uses firmware lowercase mapping.
 * @param needle Optional nonempty NUL-terminated search string; NULL selects ordinary scanning.
 * @return Emitted/consumed count, or one/zero for search success/failure.
 */
uint32_t TextScanRead(TextScan_t* position, uint32_t count, uint8_t* output,
                      bool reverse, TextByteMode_e mode, bool skip_hidden,
                      bool skip_protected, bool expand_forms, bool case_sensitive,
                      const char* needle);

/** @brief Delete a forward range from a position.
 * @param position Required scan state; traversal uses a copy of it.
 * @param count Maximum bytes in the selected mode.
 * @param mode Encoded bytes or plain converted bytes.
 * @param skip_hidden Preserve hidden regions.
 * @param skip_protected Preserve protected regions.
 * @return Zero or a TEXT_ERROR_* status; read-only descriptors reject the operation.
 */
int32_t TextScanDelete(TextScan_t* position, uint32_t count, TextByteMode_e mode,
                       bool skip_hidden, bool skip_protected);

/** @brief Insert text at a position without advancing that position.
 * @param position Required state; its editable file must be current.
 * @param data Source bytes. Encoded mode requires writable valid encoded text;
 * the ROM can adjust its boundary region flags. Plain mode escapes reserved bytes.
 * @param length Source byte count; no NUL is required or appended.
 * @param mode Encoded or plain source bytes.
 * @return Zero or a TEXT_ERROR_* value.
 */
int32_t TextScanInsert(TextScan_t* position, void* data, uint32_t length, TextByteMode_e mode);

/** @brief Move the editor caret to a scan position.
 * @param position Source position. Editable mode avoids placing the caret inside the internal gap.
 * @param draw_selection True to update selection drawing after layout.
 * @return Zero or a layout error. Read-only mode copies the offset without relayout.
 */
int32_t TextEditorSetScanPosition(TextScan_t* position, bool draw_selection);

/** @brief Replace an editable Neo editor's fonts.
 *
 * AS3000 3.01 returns TEXT_ERROR_UNIMPLEMENTED and ignores the arguments.
 * @param editor Initialized editable Neo editor.
 * @param font0 Optional replacement for font slot 0; NULL preserves it.
 * @param font1 Optional replacement for font slot 1.
 * @param font2 Optional replacement for font slot 2.
 * @param font3 Optional replacement for font slot 3.
 * @param draw_selection True to redraw selection after successful layout.
 * @return Zero, a layout error, TEXT_ERROR_READ_ONLY, or TEXT_ERROR_UNIMPLEMENTED.
 */
int32_t TextEditorSetFonts(TextEditor_t* editor, FontHeader_t* font0, FontHeader_t* font1,
                           FontHeader_t* font2, FontHeader_t* font3, bool draw_selection);

/** @brief Save the editor viewport in the current file's position.
 *
 * Ensures editable gap space first; does not remove the gap from the raw file.
 * @param editor Initialized descriptor; read-only mode does nothing.
 * @return Zero or a TEXT_ERROR_* value.
 */
int32_t TextEditorSavePosition(TextEditor_t* editor);

/** @brief Initialize native transfer state for an editor.
 * @param editor Borrowed initialized descriptor.
 * @param transfer Required writable transfer record.
 */
void TextEditorTransferInit(TextEditor_t* editor, TextTransfer_t* transfer);

/** @brief Handle a native editor transfer message.
 * @param transfer Record initialized by TextEditorTransferInit().
 * @param message Full native message code; the transport prefix is significant.
 * @param param Native transport request block, interpreted according to message. NULL only for paths that do not use it.
 * @param reply Required 32-bit reply output, cleared before dispatch.
 * @return True if consumed, false otherwise. Consumption is distinct from reply status.
 */
bool TextEditorTransferHandleMessage(TextTransfer_t* transfer, Message_e message, void* param, uint32_t* reply);

/** @brief Calculate native text statistics over a raw half-open range.
 *
 * Requires a valid file; an unresolved file does not initialize every output.
 * Every output is optional. Word counts advance at separators, so a final
 * unterminated word is not counted. Paragraph/sentence counts follow firmware
 * rules and can include a trailing empty paragraph or pending sentence boundary.
 * The routine supplies no defined return-register value.
 * @param file_id File to read.
 * @param start First raw byte offset; must be within the file.
 * @param end Exclusive raw end, clamped to the file length.
 * @param characters Optional ordinary converted character count, excluding separate form output.
 * @param separators Optional whitespace/separator count output.
 * @param words Optional separator-terminated word count output.
 * @param paragraphs Optional native paragraph count output.
 * @param newlines Optional native carriage-return count output.
 * @param sentences Optional native sentence count output.
 * @param units_2048 Optional eight-byte double output: character count divided by 2048.
 * @param form_characters Optional embedded-form character count. Nonnull enables form evaluation.
 */
void TextGetStatistics(uint16_t file_id, uint32_t start, uint32_t end, uint32_t* characters,
                       uint32_t* separators, uint32_t* words, uint32_t* paragraphs,
                       uint32_t* newlines, uint32_t* sentences, double* units_2048,
                       uint32_t* form_characters);

/** @brief Parse and optionally reset a native form.
 *
 * Selects the file, can normalize line endings and append generated FORMDATA,
 * and can change the screen. This is a firmware form dialect, not general HTML.
 * @param file_id File containing form markup, optionally inside encoded region markers.
 * @param form Required writable state.
 * @param reset True to clear the descriptor and initialize focus/layout; false reparses existing state.
 * @return Zero, TEXT_ERROR_INVALID_FORM, another text status, or a negative filesystem error.
 */
int32_t TextFormInit(uint16_t file_id, TextForm_t* form, bool reset);

/** @brief Handle native form navigation and editing.
 * @param form Valid initialized form state.
 * @param message Full applet message code.
 * @param param Message-specific scalar, such as a key word or character byte.
 * @param context Forwarded native context; pass zero without a workflow requiring it.
 * @param status Required operation-status output, cleared before handling.
 * @return True if consumed, false if unhandled.
 */
bool TextFormHandleMessage(TextForm_t* form, Message_e message, uint32_t param, uint32_t context, int32_t* status);

/** @brief Initialize transfer ranges from a parsed form.
 * @param transfer Required writable transfer state.
 * @param form Borrowed initialized form descriptor.
 */
void TextFormTransferInit(TextTransfer_t* transfer, TextForm_t* form);

/** @brief Handle a native form transfer message.
 * @param transfer Record initialized by TextFormTransferInit().
 * @param message Full native transport message.
 * @param param Message-specific native request block.
 * @param reply Required reply output, cleared before dispatch.
 * @return True if consumed, false otherwise; reply supplies protocol status.
 */
bool TextFormTransferHandleMessage(TextTransfer_t* transfer, Message_e message, void* param, uint32_t* reply);

/** @brief Rebuild a form's generated data from its defaults.
 *
 * Removes generated data and reparses the retained markup. File contents and
 * layout can change. No defined return value is supplied by the entry point.
 * @param form Valid initialized descriptor for the current file.
 */
void TextFormReset(TextForm_t* form);

/** @brief Search rendered form text from a scroll row.
 * @param form Valid initialized form; a match updates its top_row.
 * @param start_row First rendered row to examine.
 * @param needle Required nonempty NUL-terminated text; comparison uses firmware lowercase conversion.
 * @return True on a match, false when rendered text is exhausted. Only D0.b is defined.
 */
bool TextFormFind(TextForm_t* form, uint32_t start_row, const char* needle);

/** @brief Clear editable content while preserving an embedded form.
 *
 * Leaves a 512-byte internal gap in the file, plus any retained form region.
 * This is not equivalent to setting the raw file length to zero.
 * @param editor Initialized editor for the current file.
 * @return Zero. Internal failures are not reliably propagated by this entry.
 */
int32_t TextEditorClear(TextEditor_t* editor);

#endif
