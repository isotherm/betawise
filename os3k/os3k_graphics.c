#include "os3k.h"

void DrawPixel(uint16_t x, uint16_t y, bool black)
{
    RasterOp(x, y, 1, 1, NULL, black ? ROP_BLACKNESS : ROP_WHITENESS);
}

void DrawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, bool black)
{
    int32_t x = x0, y = y0;
    int32_t dx = x1 >= x0 ? (int32_t)x1 - x0 : (int32_t)x0 - x1;
    int32_t dy = y1 >= y0 ? (int32_t)y0 - y1 : (int32_t)y1 - y0;
    int32_t sx = x0 < x1 ? 1 : -1;
    int32_t sy = y0 < y1 ? 1 : -1;
    int32_t error = dx + dy;

    for (;;) {
        DrawPixel(x, y, black);
        if (x == x1 && y == y1)
            break;

        int32_t step = 2 * error;
        if (step >= dy) {
            error += dy;
            x += sx;
        }
        if (step <= dx) {
            error += dx;
            y += sy;
        }
    }
}
