#define BETAWISE_LIB
#include "os3k.h"

void _OS3K_ClearScreen();
void _OS3K_SetCursor(uint8_t row, uint8_t col, CursorMode_e cursor_mode);
char _OS3K_TranslateKeyToChar(KeyMod_e key);
uint32_t _OS3K_CallSysInt(uint32_t unused_zero, SysInt_e info, void* output);
int _OS3K_fputc(int c, FILE* stream);

struct Cursor_t {
    FontHeader_t* font;
    uint8_t col_count;
    uint8_t row_count;
    uint16_t y_top;
    uint16_t x_left;
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
            gd->font = NULL;
            _OS3K_CallSysInt(0, SYS_INT_GET_SYSTEM_FONT_PTR, &gd->font);
            _OS3K_CallSysInt(0, SYS_INT_GET_LCD_WIDTH, &scratch);
            gd->lcd_width = scratch;
            _OS3K_CallSysInt(0, SYS_INT_GET_LCD_HEIGHT, &scratch);
            gd->lcd_height = scratch & ~1;
            break;

        case MSG_SETFOCUS:
            _OS3K_ClearScreen();
            if(!gd->font) {
                break;
            }
            // Initialize screen and font.
            appletIndex = AppletFindById(0xA1F0);
            if(appletIndex > 0) {
                AppletSendMessage(appletIndex, 0x1000002, (uint32_t)&gd->font, status);
            }
            // Use constant integers to avoid long division.
            gd->col_count = (uint16_t)gd->lcd_width / 6;
            gd->row_count = gd->font->height == 16 ? (gd->lcd_height / 16) : (gd->lcd_height / 8);
            break;

        case MSG_KILLFOCUS:
            if(gd->font) {
                _LCD_SetScreenRoll(0);
            }
            break;
    };

    // Let the applet process the message.
    ProcessMessage(message, param, status);
}

void ClearScreen() {
    _OS3K_ClearScreen();
    if(gd->font) {
        gd->row = gd->col = 1;
        gd->roll = 0;
        _LCD_SetScreenRoll(gd->roll);
    }
}

void SetCursor(uint8_t row, uint8_t col, CursorMode_e cursor_mode) {
    if(!gd->font) {
        _OS3K_SetCursor(row, col, cursor_mode);
        return;
    }

    bool clear_row = false;
    SetCursorMode(CURSOR_MODE_HIDE);
    if(row < 1 || row > gd->row_count) {
        if(row < 1) {
            row = 1;
            gd->roll -= gd->font->height;
            if(gd->roll < 0) {
                gd->roll += gd->lcd_height;
            }
        } else {
            row = gd->row_count;
            gd->roll += gd->font->height;
            if(gd->roll >= gd->lcd_height) {
                gd->roll -= gd->lcd_height;
            }
        }
        _LCD_SetScreenRoll(gd->roll);
        clear_row = true;
    }
    gd->row = row;
    gd->col = col;
    CURSOR_STRUCT->font = gd->font;
    CURSOR_STRUCT->x = (col - 1) * gd->font->max_width;
    if(CURSOR_STRUCT->x == gd->lcd_width) {
        CURSOR_STRUCT->x--;
    }
    CURSOR_STRUCT->y = ((row - 1) * gd->font->height) + gd->roll;
    if(CURSOR_STRUCT->y >= gd->lcd_height) {
        CURSOR_STRUCT->y -= gd->lcd_height;
    }
    if(clear_row) {
        RasterOp(CURSOR_STRUCT->x, CURSOR_STRUCT->y, gd->lcd_width, gd->font->height, NULL, ROP_WHITENESS);
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
    if(gd->font) {
        if(info == SYS_INT_GET_ROW_HEIGHT) {
            *(uint32_t*)output = gd->font->height;
            return 0;
        } else if(info == SYS_INT_GET_ROW_COUNT) {
            *(uint32_t*)output = gd->row_count;
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
    if(!gd->font) {
        GetCursorPos(&gd->row, &gd->col);
    }
    c = (uint8_t)c;
    switch(c) {
        case '\a':
            if(!gd->font) {
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
            if(gd->col > 1) {
                SetCursor(gd->row, gd->col - 1, cursor_mode);
            } else {
                SetCursor(gd->row - 1, gd->col_count, cursor_mode);
            }
            return c;
        case '\n':
            if(!gd->font) {
                break;
            }
            // Assume carriage return with line feeds.
            SetCursor(gd->row + 1, 1, cursor_mode);
            return c;
        case '\r':
            SetCursor(gd->row, 1, cursor_mode);
            return c;
    }
    
    if(!gd->font) {
        return _OS3K_fputc(c, stream);
    }

    short offset = gd->font->max_bytes * c;
    const uint8_t* bitmap = gd->font->bitmap_data + offset;
    if(CURSOR_STRUCT->x == (gd->lcd_width - 1)) {
        SetCursor(gd->row + 1, 1, CURSOR_MODE_HIDE);
    } else {
        SetCursorMode(CURSOR_MODE_HIDE);
    }
    DrawBitmap(CURSOR_STRUCT->x, CURSOR_STRUCT->y, gd->font->max_width, gd->font->height, bitmap);
    CURSOR_STRUCT->x += gd->font->max_width;
    if(CURSOR_STRUCT->x == gd->lcd_width) {
        CURSOR_STRUCT->x--;
    }
    gd->col++;
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
