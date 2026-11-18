#include "os3k.h"
#include "../NeoFontTerminal/glyphs.h"

APPLET_HEADER_BEGIN
    APPLET_ID(0xA1CB)
    APPLET_NAME("UCBLogo")
    APPLET_INFO("Berkeley Logo port, GPLv3 or later")
    APPLET_VERSION(0, 1, "0")
    APPLET_LANGUAGE_EN_US
APPLET_HEADER_END

enum { WIDTH = 264, SPLIT = 132, HEIGHT = 64, COLS = 22, ROWS = HEIGHT / GLYPH_HEIGHT };
static uint8_t transcript[ROWS][COLS];
static char source[128];
static unsigned length, row, column, top_row;
static int pen_x, pen_y;
void neo_eval(char *text);
void neo_toggle_turtle(void);

static unsigned lcd_y(unsigned y)
{
    return ((top_row + y) % ROWS) * GLYPH_HEIGHT;
}

static void draw_glyph(unsigned y, unsigned x, uint8_t ch)
{
    DrawBitmap(x * GLYPH_WIDTH, lcd_y(y), GLYPH_WIDTH,
               GLYPH_HEIGHT, GLYPHS[ch ? ch : ' ']);
}

static void redraw_transcript(void)
{
    for (unsigned y = 0; y < ROWS; ++y)
        for (unsigned x = 0; x < COLS; ++x)
            draw_glyph(y, x, transcript[y][x]);
}

static void newline(void)
{
    column = 0;
    if (row + 1 < ROWS) {
        ++row;
    } else {
        memmove(transcript[0], transcript[1], (ROWS - 1) * COLS);
        memset(transcript[row], 0, COLS);
        // Clear the old top page before the controller exposes it at the bottom.
        RasterOp(0, lcd_y(0), SPLIT, GLYPH_HEIGHT, NULL, ROP_WHITENESS);
        top_row = (top_row + 1) % ROWS;
        LCD_CMD_REG_LEFT = LCD_CMD_START_LINE(lcd_y(0));
    }
}

static void redraw(void)
{
    // Keep the end of a long input visible and leave a cell for the cursor.
    unsigned visible = COLS - 3;
    unsigned first = length > visible ? length - visible : 0;
    unsigned cursor = 2 + length - first;
    draw_glyph(row, 0, '?');
    draw_glyph(row, 1, ' ');
    for (unsigned x = 2; x < COLS; ++x)
        draw_glyph(row, x, x < cursor ? source[first + x - 2] : ' ');
    RasterOp(cursor * GLYPH_WIDTH, lcd_y(row), 1,
             GLYPH_HEIGHT, NULL, ROP_BLACKNESS);
}

void neo_clear(void)
{
    RasterOp(SPLIT, 0, WIDTH - SPLIT, HEIGHT, NULL, ROP_WHITENESS);
    DrawLine(SPLIT, 0, SPLIT, HEIGHT - 1, true);
}

void neo_move(int x, int y)
{
    pen_x = x;
    pen_y = y;
}

void neo_cursor_edge(int x0, int y0, int x, int y)
{
    int dx = x >= x0 ? x - x0 : x0 - x;
    int dy = y >= y0 ? y0 - y : y - y0;
    int sx = x0 < x ? 1 : -1, sy = y0 < y ? 1 : -1;
    int error = dx + dy;
    for (;;) {
        // Each edge excludes its end so adjacent edges invert a vertex once.
        if (x0 == x && y0 == y)
            break;
        if (x0 >= 1 && x0 < SPLIT && y0 >= 0 && y0 < HEIGHT)
            RasterOp(SPLIT + x0, y0, 1, 1, NULL, ROP_DSTINVERT);
        int step = 2 * error;
        if (step >= dy) {
            error += dy;
            x0 += sx;
        }
        if (step <= dx) {
            error += dx;
            y0 += sy;
        }
    }
}

void neo_line(int x, int y, int hidden, int turtle)
{
    int x0 = pen_x, y0 = pen_y;
    neo_move(x, y);
    if (hidden || turtle || x0 < 1 || x0 >= SPLIT || y0 < 0 ||
        y0 >= HEIGHT || x < 1 || x >= SPLIT || y < 0 || y >= HEIGHT)
        return;
    DrawLine(SPLIT + x0, y0, SPLIT + x, y, true);
}

void neo_output(int ch)
{
    if (ch == '\n') {
        newline();
    } else if (ch >= ' ' && ch <= 255) {
        if (column == COLS)
            newline();
        transcript[row][column] = ch;
        draw_glyph(row, column++, ch);
    }
}

static void run(void)
{
    // Replace the editable row (including its cursor) with the transcript.
    RasterOp(0, lcd_y(row), SPLIT, GLYPH_HEIGHT, NULL, ROP_WHITENESS);
    neo_output('?');
    neo_output(' ');
    for (unsigned i = 0; i < length; ++i)
        neo_output(source[i]);
    newline();

    if (length)
        neo_eval(source);
    if (column)
        newline();
    length = 0;
    source[0] = 0;
}

void ProcessMessage(Message_e message, uint32_t param, uint32_t *status)
{
    *status = 0;
    if (message == MSG_SETFOCUS) {
        SetCursorMode(CURSOR_MODE_HIDE);
        top_row = 0;
        LCD_CMD_REG_LEFT = LCD_CMD_START_LINE(0);
        redraw_transcript();
        neo_clear();
        neo_toggle_turtle();
    } else if (message == MSG_KILLFOCUS) {
        LCD_CMD_REG_LEFT = LCD_CMD_START_LINE(0);
        return;
    } else if (message == MSG_CHAR) {
        if (param == '\r' || param == '\n') {
            neo_toggle_turtle();
            run();
            neo_toggle_turtle();
        } else if (param >= 32 && param <= 126 && length + 1 < sizeof(source)) {
            source[length++] = param;
            source[length] = 0;
        }
    } else if (message == MSG_KEY) {
        switch (param & ~KEY_MOD_CAPS_LOCK) {
        case KEY_BACKSPACE:
            if (length)
                source[--length] = 0;
            break;
        case KEY_MOD_CTRL | KEY_U:
            length = 0;
            source[0] = 0;
            break;
        default:
            return;
        }
    } else {
        return;
    }
    redraw();
}
