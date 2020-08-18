#include "betawise.h"
#include "glyphs.h"

APPLET_HEADER_BEGIN
    APPLET_FLAGS(APPLET_FLAG_FONT | APPLET_FLAG_HIDDEN)
    APPLET_ID(0xA1F0)
    APPLET_FONT_NAME("Terminal (8 lines)")
    APPLET_INFO("Copyright (c) 2020 Alpaxo Software")
    APPLET_VERSION(0, 0, 5)
    APPLET_LANGUAGE_EN_US
APPLET_HEADER_END

struct gd_t {
    uint8_t height;
    uint8_t maxWidth;
    uint8_t maxBytes;
    uint8_t padding;
    const uint8_t* pWidthTable;
    const uint16_t* pOffsetTable;
    const uint8_t* pBitmapData;
};

void ProcessMessage(uint32_t message, uint32_t param, uint32_t* status) {
    *status = 0;
    switch(message) {
        case 0x1000001:
            *(char**)param = APPLET_FONT_NAME_PTR;
            break;
        case 0x1000002:
            *(void**)param = gd;
            break;
        case 0x1000003:
        case 0x1000004:
        case 0x1000005:
            *(void**)param = 0;
            break;
        case 0x1000006:
            *(void**)param = gd;
        case MSG_INIT:
            gd->height = GLYPH_HEIGHT;
            gd->maxWidth = GLYPH_WIDTH;
            gd->maxBytes = GLYPH_BYTES;
            gd->pWidthTable = 0;
            gd->pOffsetTable = 0;
            gd->pBitmapData = GLYPHS[0];
            break;
    }
}
