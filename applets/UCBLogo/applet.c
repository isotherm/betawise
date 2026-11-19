#include "os3k.h"

APPLET_HEADER_BEGIN
    APPLET_ID(0xA1CB)
    APPLET_NAME("UCBLogo")
    APPLET_INFO("Berkeley Logo port, GPLv3 or later")
    APPLET_VERSION(0, 1, "0")
    APPLET_LANGUAGE_EN_US
APPLET_HEADER_END

enum { WIDTH = 264, SPLIT = 132, HEIGHT = 64, MAX_COLS = 22, MAX_ROWS = 8 };
static unsigned font_width = 6, font_height = 8, text_cols = MAX_COLS, text_rows = MAX_ROWS;
static uint8_t transcript[MAX_ROWS][MAX_COLS];
static char source[128];
static char definition_header[sizeof(source)];
static char definition_body[1024];
static unsigned definition_length;
static bool defining;
static unsigned length, row, column, top_row;
static int pen_x, pen_y;
void neo_eval(char *text, const char *body);
void neo_toggle_turtle(void);

static unsigned lcd_y(unsigned y)
{
    return ((top_row + y) % text_rows) * font_height;
}

static void draw_glyph(unsigned y, unsigned x, uint8_t ch)
{
    SetCursor(lcd_y(y) / font_height + 1, x + 1, CURSOR_MODE_HIDE);
    putchar(ch ? ch : ' ');
}

static void redraw_transcript(void)
{
    for (unsigned y = 0; y < text_rows; ++y)
        for (unsigned x = 0; x < text_cols; ++x)
            draw_glyph(y, x, transcript[y][x]);
}

static void newline(void)
{
    column = 0;
    if (row + 1 < text_rows) {
        ++row;
    } else {
        memmove(transcript[0], transcript[1], (text_rows - 1) * MAX_COLS);
        memset(transcript[row], 0, MAX_COLS);
        // Clear the old top page before the controller exposes it at the bottom.
        RasterOp(0, lcd_y(0), SPLIT, font_height, NULL, ROP_WHITENESS);
        top_row = (top_row + 1) % text_rows;
        LCD_CMD_REG_LEFT = LCD_CMD_START_LINE(lcd_y(0));
    }
}

static void redraw(void)
{
    // Keep the end of a long input visible and leave a cell for the cursor.
    unsigned visible = text_cols - 3;
    unsigned first = length > visible ? length - visible : 0;
    unsigned cursor = 2 + length - first;
    draw_glyph(row, 0, defining ? '>' : '?');
    draw_glyph(row, 1, ' ');
    for (unsigned x = 2; x < text_cols; ++x)
        draw_glyph(row, x, x < cursor ? source[first + x - 2] : ' ');
    RasterOp(cursor * font_width, lcd_y(row), 1,
             font_height, NULL, ROP_BLACKNESS);
}

void neo_cleartext(void)
{
    RasterOp(0, 0, SPLIT, HEIGHT, NULL, ROP_WHITENESS);
    memset(transcript, 0, sizeof(transcript));
    row = column = top_row = 0;
    LCD_CMD_REG_LEFT = LCD_CMD_START_LINE(0);
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
        if (column == text_cols)
            newline();
        transcript[row][column] = ch;
        draw_glyph(row, column++, ch);
    }
}

/* Match a complete first word, ignoring case and leading whitespace. */
static const char *after_word(const char *line, const char *word)
{
    while (*line == ' ' || *line == '\t')
        ++line;
    while (*word) {
        if ((*line | 0x20) != *word)
            return NULL;
        ++line;
        ++word;
    }
    if (*line && *line != ' ' && *line != '\t' && *line != ';')
        return NULL;
    while (*line == ' ' || *line == '\t')
        ++line;
    return line;
}

static void submit(void)
{
    if (defining) {
        if (length + 2 > sizeof(definition_body) - definition_length) {
            defining = false;
            const char *message = "Definition too long";
            while (*message)
                neo_output(*message++);
            return;
        }
        memcpy(definition_body + definition_length, source, length);
        definition_length += length;
        definition_body[definition_length++] = '\n';
        definition_body[definition_length] = 0;
        const char *tail = after_word(source, "end");
        if (tail && (!*tail || *tail == ';')) {
            defining = false;
            neo_eval(definition_header, definition_body);
        }
    } else {
        const char *tail = after_word(source, "to");
        if (tail && *tail && *tail != ';') {
            memcpy(definition_header, source, length + 1);
            definition_length = 0;
            defining = true;
        } else if (length) {
            neo_eval(source, NULL);
        }
    }
}

static void run(void)
{
    // Replace the editable row (including its cursor) with the transcript.
    RasterOp(0, lcd_y(row), SPLIT, font_height, NULL, ROP_WHITENESS);
    neo_output(defining ? '>' : '?');
    neo_output(' ');
    for (unsigned i = 0; i < length; ++i)
        neo_output(source[i]);
    newline();

    submit();
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
        FontHeader_t *font = NULL;
        CallSysInt(0, SYS_INT_GET_SYSTEM_FONT_PTR, &font);
        uint8_t terminal = AppletFindById(0xA1F0);
        if (terminal > 0)
            AppletSendMessage(terminal, 0x1000002, (uint32_t)&font, status);
        if (font) {
            font_width = font->max_width;
            font_height = font->height;
        }
        text_cols = SPLIT / font_width;
        if (text_cols > MAX_COLS) text_cols = MAX_COLS;
        text_rows = HEIGHT / font_height;
        if (text_rows > MAX_ROWS) text_rows = MAX_ROWS;
        if (row >= text_rows) row = text_rows - 1;
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
        case KEY_MOD_CTRL | KEY_C:
            defining = false;
            /* Fall through to clear the current input. */
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
