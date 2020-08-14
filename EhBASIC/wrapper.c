#include "betawise.h"

extern void LAB_COLD();

APPLET_HEADER_BEGIN
    APPLET_ID(0xA1BA)
    APPLET_NAME("EhBASIC")
    APPLET_INFO("Copyright(C) 2002-12 Lee Davison")
    APPLET_VERSION(0, 0, 3)
    APPLET_LANGUAGE_EN_US
APPLET_HEADER_END

struct gd_t {
    uint8_t row;
    uint8_t col;
};

#define LCD_CMD_LEFT (*(volatile uint8_t*)0x1008000)
#define LCD_CMD_RIGHT (*(volatile uint8_t*)0x1000000)

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
            LCD_CMD_LEFT = LCD_CMD_RIGHT = 0x40 | ((gd->row & 3) << 4);
            break;
        case '\r':
            gd->col = 1;
            break;
        default:
            PutChar(c);
            gd->col++;
            break;
    }
    SetCursor(gd->row, gd->col, CURSOR_HIDE);
}

char GetCharWrapper(char wait) {
    char c = 0;
    if(wait) {
        SetCursorMode(CURSOR_SHOW);
    }
    if(wait || IsKeyReady()) {
        c = getchar();
    }
    SetCursorMode(CURSOR_HIDE);
    return c;
}

void ProcessMessage(uint32_t message, uint32_t param, uint32_t* status) {
    *status = 0;
    switch(message) {
        case MSG_INIT:
            break;

        case MSG_SETFOCUS:
            ClearScreen();
            gd->row = 4;
            PutCharWrapper('\r');
            LAB_COLD();
            break;

        case MSG_KILLFOCUS:
            // Restore vertical viewport offset to 0.
            LCD_CMD_LEFT = LCD_CMD_RIGHT = 0x40;
            break;
    }
}
