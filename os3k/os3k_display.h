#ifndef _OS3K_DISPLAY_H_
#define _OS3K_DISPLAY_H_

/** @file os3k_display.h
 * @brief Display, cursor, graphics, and font interfaces.
 */

#include <stdbool.h>
#include <stdint.h>

/** @brief Cursor visibility commands accepted by the ROM. */
typedef enum _CursorMode_e {
    CURSOR_MODE_HIDE = 0x0C,
    CURSOR_MODE_SHOW = 0x0F
} CursorMode_e;

/** @brief One-based byte coordinates used by DrawTextSpan().
 *
 * The ABI stores these two bytes in row, column order. Span traversal wraps
 * after column 40, including on Neo.
 */
typedef struct _TextPosition_t {
    uint8_t row; /**< One-based text row. */
    uint8_t col; /**< One-based text column, from 1 through 40 for span traversal. */
} TextPosition_t;

/** @brief Rendering policy for DrawTextSpan(). */
typedef enum _TextSpanMode_e {
    TEXT_SPAN_HIGHLIGHT = 0, /**< Highlight the span using the model's selection renderer. */
    TEXT_SPAN_RESTORE = 1 /**< Redraw the span normally from its text shadow. */
} TextSpanMode_e;

/** @brief Neo raster operations over one-bit pixels.
 *
 * A set bit represents black. Source-based operations use the packed bitmap
 * supplied to RasterOp(). Capture writes pixels into that bitmap.
 */
typedef enum _RopCode_e {
    ROP_CAPTURE = 1, /**< Copy display pixels into the bitmap. */
    ROP_DSTINVERT = 2, /**< Invert the destination pixels. */
    ROP_SRCINVERT = 3, /**< Exclusive-OR source and destination. */
    ROP_NOTSRCCOPY = 4, /**< Write the inverted source. */
    ROP_WHITENESS = 5, /**< Clear destination bits to white. */
    ROP_BLACKNESS = 6, /**< Set destination bits to black. */
    ROP_SRCPAINT = 7 /**< Inclusive-OR source and destination. */
} RopCode_e;

/** @brief Clear the display and move the text cursor to row 1, column 1.
 *
 * Resets Betawise's Neo scrolling state.
 */
void ClearScreen(void);

/** @brief Set the one-based text position and cursor visibility.
 *
 * Betawise uses its selected Neo font and scrolls
 * when the row passes its text area.
 * @param row One-based text row.
 * @param col One-based text column.
 * @param cursor_mode Cursor visibility.
 */
void SetCursor(uint8_t row, uint8_t col, CursorMode_e cursor_mode);

/** @brief Read the ROM text position.
 * @param row Output for the one-based ROM row.
 * @param col Output for the one-based ROM column.
 * @note Betawise's custom Neo output keeps its own text position.
 */
void GetCursorPos(uint8_t* row, uint8_t* col);

/** @brief Center a string on a display row.
 * @param row One-based display row.
 * @param str NUL-terminated text.
 */
void PutStringCentered(uint8_t row, const char* str);

/** @brief Draw one character using the ROM text renderer.
 *
 * On AS3000 3.01 the column advances without automatic
 * line wrapping. Rendering requires a valid row and column.
 * @param c Character code.
 */
void PutChar(char c);

/** @brief Draw a string by passing each byte to PutChar().
 * @param str NUL-terminated text.
 */
void PutStringRaw(const char* str);

/** @brief Set ROM cursor visibility.
 * @param cursor_mode Visibility command.
 */
void SetCursorMode(CursorMode_e cursor_mode);

/** @brief Read the ROM cursor visibility command.
 * @param cursor_mode Output storage for one CursorMode_e byte with -fshort-enums.
 */
void GetCursorMode(CursorMode_e* cursor_mode);

/** @brief Replace an inclusive column interval with spaces.
 * @param row One-based display row.
 * @param col_first First column to clear.
 * @param col_last Last column to clear, inclusive.
 */
void ClearRowCols(uint8_t row, uint8_t col_first, uint8_t col_last);

/** @brief Clear an inclusive range of display rows.
 * @param row_first First one-based row.
 * @param row_last Last one-based row, inclusive.
 */
void ClearRows(uint8_t row_first, uint8_t row_last);

/** @brief Save the ROM display into its single shared screen snapshot.
 *
 * AS3000 3.01 captures all 160 LCD character bytes and
 * the ROM cursor row, column, and visibility into the text shadow. Neo
 * captures LCD pixels, its live text grid, logical and pixel cursor positions,
 * visibility, and font pointer. A later save replaces the earlier snapshot;
 * ROM dialogs can use the same storage. Betawise's private display state is
 * outside this snapshot.
 */
void SaveScreen(void);

/** @brief Restore the ROM's shared screen snapshot.
 *
 * Requires a preceding SaveScreen(). AS3000 3.01 writes
 * the text shadow bytes directly to the LCD and restores cursor metadata.
 * Neo restores the captured pixels, live text grid, cursor state, and font.
 * This does not consume the snapshot, so restoration can be repeated. On
 * AS3000 3.01, PutCharToShadow() and InvalidateTextShadow() alter the character
 * data this restores. Neo's captured pixels are separate from its text shadow.
 */
void RestoreScreen(void);

/** @brief Draw text with line-feed handling.
 *
 * On AS3000 3.01 a line feed moves to column 1 of the
 * next row with the cursor hidden. Other bytes go through PutChar().
 * @param str NUL-terminated text.
 */
void PutString(const char* str);

/** @brief Mark the ROM text shadow as unknown by filling cells with 0xFF.
 *
 * AS3000 3.01 fills its four rows of 40 cells. Neo fills
 * the configured text rows and columns in its shadow grid. It does not draw
 * on the LCD, move the cursor, or change the saved cursor metadata.
 * DrawTextSpan() treats unknown cells as spaces when redrawing normally.
 */
void InvalidateTextShadow(void);

/** @brief Redraw or highlight an inclusive span of text cells.
 *
 * This renders cells in place; it does not move
 * text or modify the source shadow. The first and last positions delimit a
 * row-major span, wrapping from column 40 to column 1 of the next row. The
 * ROM hides the cursor and finally assigns its logical row and column from
 * cursor_position. That final assignment does not reposition the hardware
 * text address or Neo pixel cursor to match.
 *
 * In normal mode, shadow byte 0xFF is drawn as a space. AS3000 3.01 passes other
 * bytes through its normal character translation. Highlight mode draws solid
 * character cells on AS3000 3.01, using a distinct glyph at cursor_position when
 * that cell is inside the span. Neo instead draws inverse versions of the
 * shadow characters. The Neo renderer uses the ROM system font.
 *
 * @param mode TEXT_SPAN_RESTORE (1) for normal redraw, TEXT_SPAN_HIGHLIGHT (0)
 * for highlighting. The ROM treats every byte other than 1 as highlight mode.
 * @param first First cell to draw, inclusive. Must be a valid text position.
 * @param last Last cell to draw, inclusive. Must be reachable from first by
 * advancing columns with the fixed 40-column wrap. The ROM checks no bounds.
 * @param cursor_position Final logical cursor coordinates; also selects the
 * distinct AS3000 3.01 highlight glyph if inside the span. The pointed-to positions
 * are read-only, and all three pointers must be valid.
 */
void DrawTextSpan(TextSpanMode_e mode, const TextPosition_t* first,
                  const TextPosition_t* last, const TextPosition_t* cursor_position);

/** @brief Store one byte in the text shadow at the ROM cursor position.
 *
 * Uses the current one-based ROM row and column.
 * It neither draws on the LCD nor advances the cursor. The byte is stored
 * unchanged, including zero and 0xFF. No position validation is performed.
 * On AS3000 3.01 the shadow is also the saved screen's character data; on Neo
 * it is separate from both the captured pixels and the live text grid.
 * @param c Byte to store in the shadow cell.
 */
void PutCharToShadow(uint8_t c);

/** @brief Copy a packed one-bit bitmap onto the Neo LCD.
 *
 * This call is a no-op on AS3000 3.01.
 * The Neo source contains w bytes per eight-pixel-high band. Bit 0 is the
 * top pixel of a band. The next byte is the next column. A second band
 * starts after w bytes.
 * @param x Zero-based left pixel coordinate.
 * @param y Zero-based top pixel coordinate.
 * @param w Width in pixels.
 * @param h Height in pixels.
 * @param bitmap Readable storage of w * ((h + 7) / 8) bytes.
 */
void DrawBitmap(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t* bitmap);

/** @brief Apply a one-bit raster operation to a Neo LCD rectangle.
 *
 * Requires compatible newer Neo firmware; unavailable on AS3000 3.01 and Neo 3.01.
 * Bitmap packing matches DrawBitmap(). The Neo ROM offsets y by its screen
 * roll and rejects rectangles extending beyond width 264 or height 66.
 * @param x Zero-based left pixel coordinate.
 * @param y Zero-based top pixel coordinate.
 * @param w Width in pixels.
 * @param h Height in pixels.
 * @param bitmap Output storage for capture or input for source-based modes.
 * May be NULL for invert, whiteness, and blackness.
 * @param rop Raster operation.
 */
void RasterOp(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t* bitmap, RopCode_e rop);

/** @brief Betawise wrapper that draws a pixel through RasterOp().
 * @param x Zero-based horizontal pixel coordinate.
 * @param y Zero-based vertical pixel coordinate.
 * @param black True draws black. False draws white.
 */
void DrawPixel(uint16_t x, uint16_t y, bool black);

/** @brief Betawise wrapper that draws an inclusive line through DrawPixel().
 * @param x0 First endpoint's horizontal coordinate.
 * @param y0 First endpoint's vertical coordinate.
 * @param x1 Second endpoint's horizontal coordinate.
 * @param y1 Second endpoint's vertical coordinate.
 * @param black True draws black. False draws white.
 */
void DrawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, bool black);

/** @brief Draw a percentage progress indicator on an AS3000 3.01 display row.
 *
 * A zero total produces zero percent.
 * The displayed percentage is capped at 100.
 * @param row One-based display row.
 * @param value Progress numerator.
 * @param total Progress denominator.
 */
void ProgressBar(uint8_t row, uint32_t value, uint32_t total);

/** @brief Neo font descriptor, 16 bytes under the m68k ABI.
 *
 * Glyph data uses the packed one-bit bitmap layout accepted by DrawBitmap().
 * Betawise's console renderer uses fixed-size glyph slots indexed by max_bytes.
 */
typedef struct _FontHeader_t {
    uint8_t height; /**< 0x00, glyph height in pixels. */
    uint8_t max_width; /**< 0x01, maximum glyph width in pixels. */
    uint8_t max_bytes; /**< 0x02, fixed glyph slot size used by Betawise. */
    uint8_t padding; /**< 0x03, reserved alignment byte. */
    const uint8_t* width_table; /**< 0x04, optional per-character widths. NULL denotes fixed width. */
    const uint16_t* offset_table; /**< 0x08, optional per-character bitmap offsets. */
    const uint8_t* bitmap_data; /**< 0x0C, packed glyph bitmap storage. */
} FontHeader_t;

// There are two ST7565R LCD controllers, for left and right half of screen.
// All interaction is via command registers, unless reading/writing buffer.
/** @brief Neo left controller command/status register. */
#define LCD_CMD_REG_LEFT (*(volatile uint8_t*)0x1008000)
/** @brief Neo right controller command/status register. */
#define LCD_CMD_REG_RIGHT (*(volatile uint8_t*)0x1000000)
/** @brief Build a Neo display enable command. */
#define LCD_CMD_ON(yes) (0xAE | ((yes) & 1))
/** @brief Build a Neo display polarity command. */
#define LCD_CMD_REVERSE(yes) (0xA6 | ((yes) & 1))
/** @brief Build a Neo all-pixels-on command. */
#define LCD_CMD_ALL_PIX_ON(yes) (0xA4 | ((yes) & 1))
/** @brief Build a Neo display start-line command. */
#define LCD_CMD_START_LINE(line) (0x40 | ((line) & 0x3F))
/** @brief Build a Neo page-address command. */
#define LCD_CMD_PAGE_ADDR(pageaddr) (0xB0 | ((pageaddr) & 0x0F))
/** @brief Build the high portion of a Neo column-address command. */
#define LCD_CMD_COL_ADDR_HI(coladdr) (0x10 | (((coladdr) >> 4) & 0x0F))
/** @brief Build the low portion of a Neo column-address command. */
#define LCD_CMD_COL_ADDR_LO(coladdr) (0x00 | ((coladdr) & 0x0F))
/** @brief Neo command selecting read-modify-write mode. */
#define LCD_CMD_COL_ADDR_INC 0xE0
/** @brief Neo command ending read-modify-write mode. */
#define LCD_CMD_COL_ADDR_END 0xEE
/** @brief Neo left controller data register. */
#define LCD_DATA_REG_LEFT (*(volatile uint8_t*)0x1008001)
/** @brief Neo right controller data register. */
#define LCD_DATA_REG_RIGHT (*(volatile uint8_t*)0x1000001)

/** @brief Animate and display battery capacity using the ROM's battery UI.
 *
 * Refreshes measurements and draws a remaining-capacity icon
 * at the upper right (four custom LCD glyphs on AS3000 3.01, a bitmap on Neo).
 * A zero duration also prints remaining capacity and waits for a fresh key;
 * a nonzero duration waits that many seconds instead. At exit it clears the
 * upper-right icon area and invalidates the ROM text shadow. It does not
 * restore the prior screen. AS3000 3.01 overwrites custom character slots 4–7.
 *
 * On revision zero hardware, no battery UI is drawn and the call only waits
 * duration_seconds * 100 centiseconds, including the delay routine's boundary
 * behavior for zero. The timed path requires the normal timer service.
 * @param duration_seconds Display duration in seconds; zero requests the modal key-dismissed report.
 */
void DrawBatteryStatus(uint8_t duration_seconds);

#endif
