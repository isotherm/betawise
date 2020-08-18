#!/usr/bin/env python

import sys

from PIL import Image


im = Image.open(sys.argv[1])
glyph_count = 256
img_width, glyph_height = im.size
glyph_width = img_width / glyph_count
glyph_bytes = glyph_width * int((glyph_height + 7) / 8)

glyphs = []
for i in range(glyph_count):
    bitmap = []
    for j in range(glyph_width):
        x = i*glyph_width + j
        val = 0
        for y in range(glyph_height):
            if im.getpixel((x, y)) < 128:
                val |= 1 << y
        bitmap.append('0x%02X' % (val))
    glyphs.append(bitmap)

f = open(sys.argv[2], 'w')
guard = '_%s_' % sys.argv[2].upper().replace('.', '_')
f.write('#ifndef %s\n' % guard)
f.write('#define %s\n' % guard)
f.write('\n')
f.write('#define GLYPH_HEIGHT %d\n' % glyph_height)
f.write('#define GLYPH_WIDTH %d\n' % glyph_width)
f.write('#define GLYPH_BYTES %d\n' % glyph_bytes)
f.write('\n')
f.write('const uint8_t GLYPHS[%d][%d] = {\n' % (glyph_count, glyph_bytes));
for c, bitmap in enumerate(glyphs):
    sep = ',' if c < glyph_count - 1 else ' '
    f.write('    {%s}%c // %02X\n' % (', '.join(bitmap), sep, c))
f.write('};\n')
f.write('\n')
f.write('#endif\n')
