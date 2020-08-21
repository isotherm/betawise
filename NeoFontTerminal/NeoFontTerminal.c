#include "betawise.h"
#include "glyphs.h"

APPLET_HEADER_BEGIN
    APPLET_FLAGS(APPLET_FLAG_FONT | APPLET_FLAG_HIDDEN)
    APPLET_ID(0xA1F0)
    APPLET_FONT_NAME("Terminal (8 lines)")
    APPLET_INFO("Copyright (c) 2020 Alpaxo Software")
    APPLET_VERSION(BETAWISE_VERSION_MAJOR, BETAWISE_VERSION_MINOR, BETAWISE_VERSION_REVISION)
    APPLET_LANGUAGE_EN_US
APPLET_HEADER_END

GLOBAL_DATA_BEGIN
    FontHeader_t font;
GLOBAL_DATA_END

void ProcessMessage(Message_e message, uint32_t param, uint32_t* status) {
    *status = 0;
    switch(message) {
        case 0x1000001:
            *(char**)param = APPLET_FONT_NAME_PTR;
            break;

        case 0x1000002:
            *(FontHeader_t**)param = &gd->font;
            break;

        case 0x1000003:
        case 0x1000004:
        case 0x1000005:
            *(FontHeader_t**)param = 0;
            break;

        case 0x1000006:
            *(FontHeader_t**)param = &gd->font;
        case MSG_INIT:
            gd->font.height = GLYPH_HEIGHT;
            gd->font.max_width = GLYPH_WIDTH;
            gd->font.max_bytes = GLYPH_BYTES;
            // Monospaced, so no lookup tables required.
            gd->font.width_table = 0;
            gd->font.offset_table = 0;
            gd->font.bitmap_data = GLYPHS[0];
            break;
    }
}
