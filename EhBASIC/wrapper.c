#include "betawise.h"

APPLET_HEADER_BEGIN
    APPLET_ID(0xA1BA)
    APPLET_NAME("EhBASIC")
    APPLET_INFO("Copyright(C) 2002-12 Lee Davison")
    APPLET_VERSION(BETAWISE_VERSION_MAJOR, BETAWISE_VERSION_MINOR, BETAWISE_VERSION_REVISION)
    APPLET_LANGUAGE_EN_US
APPLET_HEADER_END

GLOBAL_DATA_BEGIN
    uint16_t x;
    uint16_t y;
    uint16_t roll;
    uint8_t row;
    uint8_t col;
    uint8_t row_count;
    uint8_t col_count;
    FontHeader_t* font;
GLOBAL_DATA_END

extern void EhBASIC_Init();
extern void EhBASIC_Resume();

void BwDrawCursor() {
    uint16_t x = gd->x;
    uint16_t w = gd->font->max_width + 1;
    if(x > 0) {
        x--;
        w++;
    }
    RasterOp(x, gd->y % LCD_HEIGHT, w, gd->font->height + 1, NULL, ROP_DSTINVERT);
}

void BwClearScreen() {
    ClearScreen();
    gd->x = gd->y = gd->roll = 0;
    gd->row = gd->col = 1;
    BwDrawCursor();
}

void BwPutChar(char c) {
    BwDrawCursor();
    switch(c) {
        case '\a':
            // TODO: Handle bell.
            break;
        case '\b':
            if(gd->col > 1) {
                gd->x -= gd->font->max_width;
                gd->col--;
            } else {
                // TODO: Handle move to previous row?
            }
            break;
        case '\n':
            gd->y += gd->font->height;
            gd->row++;
            if(gd->row > gd->row_count) {
                // Adjust vertical viewport offset.
                gd->row = gd->row_count;
                gd->roll += gd->font->height;
                LCD_CMD_REG_LEFT = LCD_CMD_REG_RIGHT = 0x40 | (gd->roll & 0x3F);
            }
            RasterOp(gd->x, gd->y % LCD_HEIGHT, LCD_WIDTH, gd->font->height, NULL, ROP_WHITENESS);
            break;
        case '\r':
            gd->x = 0;
            gd->col = 1;
            break;
        default:
            DrawBitmap(gd->x, gd->y % LCD_HEIGHT, gd->font->max_width, gd->font->height, gd->font->bitmap_data + (gd->font->max_bytes * c));
            gd->x += gd->font->max_width;
            gd->col++;
            break;
    }
    BwDrawCursor();
}

char BwGetChar() {
    KeyMod_e key;
    char c = 0;

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
    return c;
}

void ProcessMessage(Message_e message, uint32_t param, uint32_t* status) {
    uint8_t appletIndex;
    uint32_t statusTemp;

    *status = 0;
    switch(message) {
        case MSG_INIT:
            EhBASIC_Init();
            break;

        case MSG_SETFOCUS:
            appletIndex = AppletFindById(0xA1F0);
            if(appletIndex > 0) {
                AppletSendMessage(appletIndex, 0x1000002, (uint32_t)&gd->font, &statusTemp);
                gd->row_count = LCD_HEIGHT / 8;
            } else {
                GetSystemInfo(0, SYS_INFO_SYSTEM_FONT_PTR, &gd->font);
                gd->row_count = LCD_HEIGHT / 16;
            }
            gd->col_count = LCD_WIDTH / 6;
            BwClearScreen();
            EhBASIC_Resume();
            break;

        case MSG_KILLFOCUS:
            // Restore vertical viewport offset to 0.
            LCD_CMD_REG_LEFT = LCD_CMD_REG_RIGHT = 0x40;
            break;
    }
}
