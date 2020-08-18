#include "betawise.h"
#include "glyphs.h"

APPLET_HEADER_BEGIN
    APPLET_FLAGS(APPLET_FLAG_FONT | APPLET_FLAG_HIDDEN)
    APPLET_ID(0xA1F0)
    APPLET_FONT_NAME("Terminal (8 lines)")
    APPLET_INFO("Copyright (c) 2020 Alpaxo Software")
    APPLET_VERSION(0, 0, 6)
    APPLET_LANGUAGE_EN_US
APPLET_HEADER_END

struct gd_t {
    struct FontHeader_t font;
};

void ProcessMessage(uint32_t message, uint32_t param, uint32_t* status) {
    *status = 0;
    switch(message) {
        case 0x1000001:
            *(char**)param = APPLET_FONT_NAME_PTR;
            break;

        case 0x1000002:
            *(struct FontHeader_t**)param = &gd->font;
            break;

        case 0x1000003:
        case 0x1000004:
        case 0x1000005:
            *(struct FontHeader_t**)param = 0;
            break;

        case 0x1000006:
            *(struct FontHeader_t**)param = &gd->font;
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
