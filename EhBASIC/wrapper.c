#include "betawise.h"

APPLET_HEADER_BEGIN
    APPLET_ID(0xA1BA)
    APPLET_NAME("EhBASIC")
    APPLET_INFO("Copyright(C) 2002-12 Lee Davison")
    APPLET_VERSION(BETAWISE_VERSION_MAJOR, BETAWISE_VERSION_MINOR, BETAWISE_VERSION_REVISION)
    APPLET_LANGUAGE_EN_US
APPLET_HEADER_END

GLOBAL_DATA_BEGIN
    uint8_t row;
    uint8_t col;
GLOBAL_DATA_END

extern void EhBASIC_Init();
extern void EhBASIC_Resume();

void PutCharWrapper(char c) {
    switch(c) {
        case '\a':
            // TODO: Handle bell.
            break;
        case '\b':
            if(gd->col > 1) {
                gd->col--;
            } else {
                // TODO: Handle move to previous row?
            }
            break;
        case '\n':
            gd->row = (gd->row & 3) + 1;
            ClearRows(gd->row, gd->row);
            // Set vertical viewport offset.
            LCD_CMD_REG_LEFT = LCD_CMD_REG_RIGHT = 0x40 | ((gd->row & 3) << 4);
            break;
        case '\r':
            gd->col = 1;
            break;
        default:
            PutChar(c);
            gd->col++;
            break;
    }
    SetCursor(gd->row, gd->col, CURSOR_MODE_HIDE);
}

char GetCharWrapper(bool wait) {
    KeyMod_e key;
    char c = 0;

    if(wait) {
        SetCursorMode(CURSOR_MODE_SHOW);
    }
    do {
        if(!IsKeyReady()) {
            ScanKeyboard();
        }
        key = GetKey(false);
        if(key == KEY_APPLETS) {
            // Invokes the applets menu and does not return.
            SYS_A25C(0x8, key);
        } else if(key == (KEY_MOD_CTRL | KEY_C)) {
            c = '\x03';
        } else if((key & 0xF0FF) == KEY_BACKSPACE) {
            c = '\b';
        } else {
            c = TranslateKeyToChar(key);
        }
    } while(!c && wait);
    SetCursorMode(CURSOR_MODE_HIDE);
    return c;
}

void ProcessMessage(Message_e message, uint32_t param, uint32_t* status) {
    *status = 0;
    switch(message) {
        case MSG_INIT:
            EhBASIC_Init();
            break;

        case MSG_SETFOCUS:
            ClearScreen();
            gd->row = 4;
            PutCharWrapper('\r');
            EhBASIC_Resume();
            break;

        case MSG_KILLFOCUS:
            // Restore vertical viewport offset to 0.
            LCD_CMD_REG_LEFT = LCD_CMD_REG_RIGHT = 0x40;
            break;
    }
}
