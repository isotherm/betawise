#include "os3k.h"

FontHeader_t* g_pCurFont;
uint16_t g_LcdWidth;
uint16_t g_LcdHeight;
int16_t g_CurLcdRoll;
uint8_t g_CurRow;
uint8_t g_CurCol;
uint8_t g_MaxRow;
uint8_t g_MaxCol;

void _OS3K_ClearScreen();
void _OS3K_SetCursor(uint8_t row, uint8_t col, CursorMode_e cursor_mode);
char _OS3K_TranslateKeyToChar(KeyMod_e key);
uint32_t _OS3K_CallSysInt(uint32_t unused_zero, SysInt_e info, void* output);
int _OS3K_fputc(int c, FILE* stream);

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

#define CURSOR_STRUCT ((struct Cursor_t*)0x5C68)

void _LCD_SetScreenRoll(uint16_t roll) {
    LCD_CMD_REG_LEFT = LCD_CMD_REG_RIGHT = LCD_CMD_START_LINE(roll);
}

void _LCD_AllPixOn(bool enable) {
    LCD_CMD_REG_LEFT = LCD_CMD_REG_RIGHT = LCD_CMD_ALL_PIX_ON(true);
}

void BwProcessMessage(Message_e message, uint32_t param, uint32_t* status) {
    uint8_t appletIndex;
    uint32_t scratch;
    extern AppletHeader_t __header;

    switch(message) {
        case MSG_INIT:
            g_pCurFont = NULL;
            _OS3K_CallSysInt(0, SYS_INT_GET_SYSTEM_FONT_PTR, &g_pCurFont);
            _OS3K_CallSysInt(0, SYS_INT_GET_LCD_WIDTH, &scratch);
            g_LcdWidth = scratch;
            _OS3K_CallSysInt(0, SYS_INT_GET_LCD_HEIGHT, &scratch);
            g_LcdHeight = scratch & ~1;
            break;

        case MSG_SETFOCUS:
            _OS3K_ClearScreen();
            if(!g_pCurFont) {
                break;
            }
            // Initialize screen and font.
            appletIndex = AppletFindById(0xA1F0);
            if(appletIndex > 0) {
                AppletSendMessage(appletIndex, 0x1000002, (uint32_t)&g_pCurFont, status);
            }
            // Use constant integers to avoid long division.
            g_MaxCol = (uint16_t)g_LcdWidth / 6;
            g_MaxRow = g_pCurFont->height == 16 ? (g_LcdHeight / 16) : (g_LcdHeight / 8);
            break;

        case MSG_KILLFOCUS:
            if(g_pCurFont) {
                _LCD_SetScreenRoll(0);
            }
            break;
    };

    // Let the applet process the message.
    ProcessMessage(message, param, status);
}

void ClearScreen() {
    _OS3K_ClearScreen();
    if(g_pCurFont) {
        g_CurRow = g_CurCol = 1;
        g_CurLcdRoll = 0;
        _LCD_SetScreenRoll(g_CurLcdRoll);
    }
}

void SetCursor(uint8_t row, uint8_t col, CursorMode_e cursor_mode) {
    if(!g_pCurFont) {
        _OS3K_SetCursor(row, col, cursor_mode);
        return;
    }

    bool clear_row = false;
    SetCursorMode(CURSOR_MODE_HIDE);
    if(row < 1 || row > g_MaxRow) {
        if(row < 1) {
            row = 1;
            g_CurLcdRoll -= g_pCurFont->height;
            if(g_CurLcdRoll < 0) {
                g_CurLcdRoll += g_LcdHeight;
            }
        } else {
            row = g_MaxRow;
            g_CurLcdRoll += g_pCurFont->height;
            if(g_CurLcdRoll >= g_LcdHeight) {
                g_CurLcdRoll -= g_LcdHeight;
            }
        }
        _LCD_SetScreenRoll(g_CurLcdRoll);
        clear_row = true;
    }
    g_CurRow = row;
    g_CurCol = col;
    CURSOR_STRUCT->pFont = g_pCurFont;
    CURSOR_STRUCT->x = (col - 1) * g_pCurFont->max_width;
    if(CURSOR_STRUCT->x == g_LcdWidth) {
        CURSOR_STRUCT->x--;
    }
    CURSOR_STRUCT->y = ((row - 1) * g_pCurFont->height) + g_CurLcdRoll;
    if(CURSOR_STRUCT->y >= g_LcdHeight) {
        CURSOR_STRUCT->y -= g_LcdHeight;
    }
    if(clear_row) {
        RasterOp(CURSOR_STRUCT->x, CURSOR_STRUCT->y, g_LcdWidth, g_pCurFont->height, NULL, ROP_WHITENESS);
    }
    SetCursorMode(cursor_mode);
}

char TranslateKeyToChar(KeyMod_e key)
{
    char c;
    c = _OS3K_TranslateKeyToChar(key);
    if(!c) {
        key &= ~KEY_MOD_CAPS_LOCK;
        if(key == KEY_APPLETS) {
            // Invokes the applets menu and does not return.
            SYS_A25C(0x8, key);
        } else if(key == (KEY_MOD_CTRL | KEY_C)) {
            c = '\x03';
        } else if(key == KEY_BACKSPACE) {
            c = '\b';
        }
    }
    return c;
}

uint32_t CallSysInt(uint32_t unused_zero, SysInt_e info, void* output) {
    if(g_pCurFont) {
        if(info == SYS_INT_GET_ROW_HEIGHT) {
            *(uint32_t*)output = g_pCurFont->height;
            return 0;
        } else if(info == SYS_INT_GET_ROW_COUNT) {
            *(uint32_t*)output = g_MaxRow;
            return 0;
        }
    }
    return _OS3K_CallSysInt(unused_zero, info, output);
}

int getchar() {
    KeyMod_e key;
    char c;

    do {
        while(!IsKeyReady()) {
            ScanKeyboard();
        }
        key = GetKey(false);
        c = TranslateKeyToChar(key);
    } while(!c);
    return c;
}

int fputc(int c, FILE* stream) {
    if(stream != stdout) {
        return _OS3K_fputc(c, stream);
    }

    CursorMode_e cursor_mode;
    GetCursorMode(&cursor_mode);
    if(!g_pCurFont) {
        GetCursorPos(&g_CurRow, &g_CurCol);
    }
    c = (uint8_t)c;
    switch(c) {
        case '\a':
            if(!g_pCurFont) {
                // TODO: Figure out how to flash AS3000 screen.
                break;
            }
            SetCursorMode(CURSOR_MODE_HIDE);
            _LCD_AllPixOn(true);
            SleepCentimilliseconds(10000);                
            _LCD_AllPixOn(false);
            SetCursorMode(cursor_mode);
            return c;
        case '\b':
            if(g_CurCol > 1) {
                SetCursor(g_CurRow, g_CurCol - 1, cursor_mode);
            } else {
                SetCursor(g_CurRow - 1, g_MaxCol, cursor_mode);
            }
            return c;
        case '\n':
            if(!g_pCurFont) {
                break;
            }
            // Assume carriage return with line feeds.
            SetCursor(g_CurRow + 1, 1, cursor_mode);
            return c;
        case '\r':
            SetCursor(g_CurRow, 1, cursor_mode);
            return c;
    }
    
    if(!g_pCurFont) {
        return _OS3K_fputc(c, stream);
    }

    short offset = g_pCurFont->max_bytes * c;
    const uint8_t* bitmap = g_pCurFont->bitmap_data + offset;
    if(CURSOR_STRUCT->x == (g_LcdWidth - 1)) {
        SetCursor(g_CurRow + 1, 1, CURSOR_MODE_HIDE);
    } else {
        SetCursorMode(CURSOR_MODE_HIDE);
    }
    DrawBitmap(CURSOR_STRUCT->x, CURSOR_STRUCT->y, g_pCurFont->max_width, g_pCurFont->height, bitmap);
    CURSOR_STRUCT->x += g_pCurFont->max_width;
    if(CURSOR_STRUCT->x == g_LcdWidth) {
        CURSOR_STRUCT->x--;
    }
    g_CurCol++;
    SetCursorMode(cursor_mode);
    return c;
}

int fputs(const char* str, FILE* stream) {
    char c;
    int res;
    while(c = *str++) {
        res = fputc((uint8_t)c, stream);
        if(res < 0) {
            return res;
        }
    }
    return 0;
}

int putchar(int c) {
    return fputc(c, stdout);
}

int puts(const char* str) {
    return fputs(str, stdout);
}
