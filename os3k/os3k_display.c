#include "os3k_display_internal.h"

typedef struct _DisplayState_t {
    FontHeader_t* font;
    FontHeader_t* system_font;
    uint16_t width;
    uint16_t height;
    int16_t roll;
    uint8_t row;
    uint8_t col;
    uint8_t max_row;
    uint8_t max_col;
    uint8_t left_padding;
    uint8_t top_padding;
} DisplayState_t;

static DisplayState_t g_Display;

struct Cursor_t {
    FontHeader_t* pFont;
    uint8_t nCols;
    uint8_t nRows;
    uint16_t yOrigin;
    uint16_t xOrigin;
    uint16_t x;
    uint16_t y;
    bool visible;
    bool pause;
};

// TODO: Resolve the Neo cursor address and layout for the running ROM version.
#define CURSOR_STRUCT ((struct Cursor_t*)0x5C68)

void _OS3K_ClearScreen(void);
void _OS3K_SetCursor(uint8_t row, uint8_t col, CursorMode_e cursor_mode);
void _OS3K_GetCursorPos(uint8_t* row, uint8_t* col);

static void _LCD_SetScreenRoll(uint16_t roll) {
    LCD_CMD_REG_LEFT = LCD_CMD_REG_RIGHT = LCD_CMD_START_LINE(roll);
}

static uint16_t ConsolePixelX(uint8_t col) {
    return (col - 1) * g_Display.font->max_width + g_Display.left_padding;
}

static uint16_t ConsolePixelY(uint8_t row) {
    uint16_t y = (row - 1) * g_Display.font->height + g_Display.roll + g_Display.top_padding;
    return y < g_Display.height ? y : y - g_Display.height;
}

static void ClearPixels(uint16_t x, uint16_t y, uint16_t width, uint16_t height) {
    uint16_t first = g_Display.height - y;
    if(height > first) {
        RasterOp(x, 0, width, height - first, NULL, ROP_WHITENESS);
        height = first;
    }
    if(height)
        RasterOp(x, y, width, height, NULL, ROP_WHITENESS);
}

static void DrawGlyph(uint16_t x, uint16_t y, const uint8_t* bitmap) {
    uint8_t width = g_Display.font->max_width;
    uint8_t height = g_Display.font->height;
    uint16_t first = g_Display.height - y;
    if(height <= first) {
        DrawBitmap(x, y, width, height, bitmap);
    } else {
        // Only the 6- and 7-pixel fonts can cross a 64-pixel roll boundary.
        uint8_t remainder[6];
        for(uint8_t col = 0; col < width; ++col)
            remainder[col] = bitmap[col] >> first;
        DrawBitmap(x, y, width, first, bitmap);
        DrawBitmap(x, 0, width, height - first, remainder);
    }
}

static void _LCD_AllPixOn(bool enable) {
    LCD_CMD_REG_LEFT = LCD_CMD_REG_RIGHT = LCD_CMD_ALL_PIX_ON(true);
}

void ClearScreen() {
    _OS3K_ClearScreen();
    if(g_Display.font) {
        g_Display.roll = 0;
        _LCD_SetScreenRoll(g_Display.roll);
        SetCursor(1, 1, CURSOR_MODE_HIDE);
    }
}

void GetCursorPos(uint8_t* row, uint8_t* col) {
    if(g_Display.font) {
        *row = g_Display.row;
        *col = g_Display.col;
    } else {
        _OS3K_GetCursorPos(row, col);
    }
}

void SetCursor(uint8_t row, uint8_t col, CursorMode_e cursor_mode) {
    if(!g_Display.font) {
        _OS3K_SetCursor(row, col, cursor_mode);
        return;
    }

    bool clear_row = false;
    SetCursorMode(CURSOR_MODE_HIDE);
    if(row < 1 || row > g_Display.max_row) {
        if(row < 1) {
            row = 1;
            g_Display.roll -= g_Display.font->height;
            if(g_Display.roll < 0) {
                g_Display.roll += g_Display.height;
            }
        } else {
            row = g_Display.max_row;
            g_Display.roll += g_Display.font->height;
            if(g_Display.roll >= g_Display.height) {
                g_Display.roll -= g_Display.height;
            }
        }
        _LCD_SetScreenRoll(g_Display.roll);
        clear_row = true;
    }
    g_Display.row = row;
    g_Display.col = col;
    CURSOR_STRUCT->pFont = g_Display.font;
    CURSOR_STRUCT->x = ConsolePixelX(col);
    if(CURSOR_STRUCT->x == g_Display.width) {
        CURSOR_STRUCT->x--;
    }
    CURSOR_STRUCT->y = ConsolePixelY(row);
    if(clear_row) {
        ClearPixels(0, CURSOR_STRUCT->y, g_Display.width, g_Display.font->height);
        uint16_t used = g_Display.max_row * g_Display.font->height;
        // In the LCD ring the bottom and top margins are adjacent.
        ClearPixels(0, ConsolePixelY(g_Display.max_row + 1),
                    g_Display.width, g_Display.height - used);
    }
    SetCursorMode(cursor_mode);
}

void _DisplayProcessMessage(Message_e message, uint32_t* status) {
    uint8_t appletIndex;
    uint32_t scratch;

    switch(message) {
        case MSG_INIT:
            g_Display.system_font = NULL;
            _OS3K_CallSysInt(0, SYS_INT_GET_SYSTEM_FONT_PTR, &g_Display.system_font);
            g_Display.font = g_Display.system_font;
            _OS3K_CallSysInt(0, SYS_INT_GET_LCD_WIDTH, &scratch);
            g_Display.width = scratch;
            // Character LCDs report columns; backspace must wrap to column 40.
            // Neo pixel widths are converted to columns when its font is set.
            if(!g_Display.font) g_Display.max_col = g_Display.width;
            _OS3K_CallSysInt(0, SYS_INT_GET_LCD_HEIGHT, &scratch);
            g_Display.height = scratch & ~1;
            break;

        case MSG_SETFOCUS:
            g_Display.font = g_Display.system_font;
            if(!g_Display.font) {
                _OS3K_ClearScreen();
                break;
            }
            // Initialize screen and font.
            appletIndex = AppletFindById(0xA1F0);
            if(appletIndex > 0) {
                FontHeader_t* font = NULL;
                AppletSendMessage(appletIndex, 0x1000002, (uint32_t)&font, status);
                if(font && font->bitmap_data &&
                   ((font->max_width == 4 && font->height == 6 && font->max_bytes == 4) ||
                    (font->max_width == 5 && font->height == 7 && font->max_bytes == 5) ||
                    (font->max_width == 6 && font->height == 8 && font->max_bytes == 6))) {
                    g_Display.font = font;
                }
            }
            // Full cells on the Neo's 264 by 64 visible raster.
            switch(g_Display.font->height) {
                case 6:
                    g_Display.max_col = 66;
                    g_Display.max_row = 10;
                    g_Display.left_padding = 0;
                    g_Display.top_padding = 2;
                    break;

                case 7:
                    g_Display.max_col = 52;
                    g_Display.max_row = 9;
                    g_Display.left_padding = 2;
                    g_Display.top_padding = 1;
                    break;

                case 8:
                    g_Display.max_col = 44;
                    g_Display.max_row = 8;
                    g_Display.left_padding = 0;
                    g_Display.top_padding = 0;
                    break;

                default:
                    g_Display.max_col = 44;
                    g_Display.max_row = 4;
                    g_Display.left_padding = 0;
                    g_Display.top_padding = 0;
                    break;
            }
            ClearScreen();
            break;

        case MSG_KILLFOCUS:
            if(g_Display.font) {
                _LCD_SetScreenRoll(0);
            }
            break;
    }
}

void _DisplayUseUnpaddedOrigin(void) {
    if(g_Display.font && (g_Display.left_padding || g_Display.top_padding)) {
        g_Display.left_padding = g_Display.top_padding = 0;
        SetCursor(1, 1, CURSOR_MODE_HIDE);
    }
}

bool _DisplayQuery(SysInt_e info, void* output) {
    if(g_Display.font) {
        if(info == SYS_INT_GET_ROW_HEIGHT) {
            *(uint32_t*)output = g_Display.font->height;
            return true;
        } else if(info == SYS_INT_GET_ROW_COUNT) {
            *(uint32_t*)output = g_Display.max_row;
            return true;
        }
    }
    return false;
}

int _DisplayPutChar(int c) {
    CursorMode_e cursor_mode;
    GetCursorMode(&cursor_mode);
    if(!g_Display.font) {
        GetCursorPos(&g_Display.row, &g_Display.col);
    }
    c = (uint8_t)c;
    switch(c) {
        case '\a':
            if(!g_Display.font) {
                // TODO: Figure out how to flash AS3000 3.01 screen.
                break;
            }
            SetCursorMode(CURSOR_MODE_HIDE);
            _LCD_AllPixOn(true);
            BusyLoop(10000);
            _LCD_AllPixOn(false);
            SetCursorMode(cursor_mode);
            return c;
        case '\b':
            if(g_Display.col > 1) {
                SetCursor(g_Display.row, g_Display.col - 1, cursor_mode);
            } else {
                SetCursor(g_Display.row - 1, g_Display.max_col, cursor_mode);
            }
            return c;
        case '\n':
            if(!g_Display.font) {
                break;
            }
            // Assume carriage return with line feeds.
            SetCursor(g_Display.row + 1, 1, cursor_mode);
            return c;
        case '\r':
            SetCursor(g_Display.row, 1, cursor_mode);
            return c;
    }

    if(!g_Display.font) {
        return _OS3K_fputc(c, stdout);
    }

    short offset = g_Display.font->max_bytes * c;
    const uint8_t* bitmap = g_Display.font->bitmap_data + offset;
    if(g_Display.col > g_Display.max_col) {
        SetCursor(g_Display.row + 1, 1, CURSOR_MODE_HIDE);
    } else {
        SetCursorMode(CURSOR_MODE_HIDE);
    }
    DrawGlyph(CURSOR_STRUCT->x, CURSOR_STRUCT->y, bitmap);
    CURSOR_STRUCT->x += g_Display.font->max_width;
    if(CURSOR_STRUCT->x == g_Display.width) {
        CURSOR_STRUCT->x--;
    }
    g_Display.col++;
    SetCursorMode(cursor_mode);
    return c;
}

void _DisplayGetSize(uint8_t* rows, uint8_t* cols) {
    if(g_Display.font) {
        *rows = g_Display.max_row;
        *cols = g_Display.max_col;
    } else {
        uint32_t n = 4;
        _OS3K_CallSysInt(0, SYS_INT_GET_ROW_COUNT, &n);
        *rows = n;
        n = 40;
        _OS3K_CallSysInt(0, SYS_INT_GET_LCD_WIDTH, &n);
        *cols = n;
    }
}

void _DisplayEraseCells(uint8_t row, uint8_t first, uint8_t last) {
    uint8_t old_row, old_col;
    CursorMode_e mode;
    GetCursorPos(&old_row, &old_col);
    GetCursorMode(&mode);
    SetCursorMode(CURSOR_MODE_HIDE);
    if(!g_Display.font) {
        ClearRowCols(row, first, last);
    } else {
        ClearPixels(ConsolePixelX(first), ConsolePixelY(row),
                    (last - first + 1) * g_Display.font->max_width, g_Display.font->height);
    }
    SetCursor(old_row, old_col, mode);
}
