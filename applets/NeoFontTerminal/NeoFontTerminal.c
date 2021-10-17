#include "os3k.h"
#include "glyphs.h"

APPLET_HEADER_BEGIN
    APPLET_FLAGS(APPLET_FLAG_FONT | APPLET_FLAG_HIDDEN)
    APPLET_ID(0xA1F0)
    APPLET_FONT_NAME("Terminal (8 lines)")
    APPLET_INFO("Copyright (c) 2020 Alpaxo Software")
    APPLET_VERSION(BETAWISE_VERSION_MAJOR, BETAWISE_VERSION_MINOR, BETAWISE_VERSION_REVISION)
    APPLET_LANGUAGE_EN_US
APPLET_HEADER_END

FontHeader_t g_pFontHeader;

void ProcessMessage(Message_e message, uint32_t param, uint32_t* status) {
    *status = 0;
    switch(message) {
        case 0x1000001:
            *(char**)param = APPLET_FONT_NAME_PTR;
            break;

        case 0x1000002:
            *(FontHeader_t**)param = &g_pFontHeader;
            break;

        case 0x1000003:
        case 0x1000004:
        case 0x1000005:
            *(FontHeader_t**)param = 0;
            break;

        case 0x1000006:
            *(FontHeader_t**)param = &g_pFontHeader;
        case MSG_INIT:
            g_pFontHeader.height = GLYPH_HEIGHT;
            g_pFontHeader.max_width = GLYPH_WIDTH;
            g_pFontHeader.max_bytes = GLYPH_BYTES;
            // Monospaced, so no lookup tables required.
            g_pFontHeader.width_table = 0;
            g_pFontHeader.offset_table = 0;
            g_pFontHeader.bitmap_data = GLYPHS[0];
            break;
    }
}
