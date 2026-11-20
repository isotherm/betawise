#!/usr/bin/env python3
"""Exact, offline NeoFontTerminal screenshot decoding. Unknown pixels fail closed."""
import argparse
from collections import defaultdict
from dataclasses import asdict, dataclass
import hashlib
import json
from pathlib import Path
import sys

from PIL import Image

FONT = Path(__file__).resolve().parents[3] / 'applets/NeoFontTerminal/glyphs.png'


@dataclass
class Reading:
    text: str
    unknown_cells: list
    ambiguous_cells: list
    cursor_columns: list
    font_sha256: str

    @property
    def clean(self):
        return not (self.unknown_cells or self.ambiguous_cells or self.cursor_columns)


def bitmap(path, size):
    """Accept native or exact integer nearest-neighbour enlargement, two colours only."""
    with Image.open(path) as source:
        im = source.convert('RGB')
    w, h = size
    scale = im.width // w
    if scale < 1 or im.size != (w * scale, h * scale):
        raise ValueError(f'{path}: expected {w}x{h} or an integer enlargement; got {im.size}')
    small = im.resize(size, Image.Resampling.NEAREST)
    if small.resize(im.size, Image.Resampling.NEAREST).tobytes() != im.tobytes():
        raise ValueError(f'{path}: enlargement contains nonuniform pixel blocks')
    colors = small.getcolors(w * h)
    if not colors or len(colors) > 2:
        raise ValueError(f'{path}: expected a two-colour lossless LCD image')
    # Luminance determines polarity; never infer background from a corner/cursor.
    ordered = sorted((c for _, c in colors), key=lambda c: 299*c[0]+587*c[1]+114*c[2])
    if len(ordered) == 1:
        # A single-colour frame cannot prove foreground/background polarity.
        raise ValueError(f'{path}: uniform frame has no verifiable LCD polarity')
    dark = ordered[0]
    return [int(c == dark) for c in small.getdata()]


def decode(path, font=FONT, region=None):
    font = Path(font)
    bits = bitmap(font, (1536, 8))
    glyphs = defaultdict(list)
    # Latin-1 printable characters only. Never invent text for control glyphs.
    for code in list(range(32, 127)) + list(range(160, 256)):
        key = tuple(bits[y*1536 + code*6 + x] for y in range(8) for x in range(6))
        glyphs[key].append(chr(code))
    screen = bitmap(path, (264, 64))
    x0, y0, width, height = region or (0, 0, 264, 64)
    if (x0 < 0 or y0 < 0 or width <= 0 or height <= 0 or x0+width > 264
            or y0+height > 64 or x0 % 6 or y0 % 8 or width % 6 or height % 8):
        raise ValueError('region must fit the LCD and align to 6x8 cells')
    unknown, ambiguous, cursors, lines = [], [], [], []
    for row in range(y0//8, (y0+height)//8):
        # Neo's visible cursor is two full-height columns, straddling cell boundaries.
        full = [x for x in range(x0, x0+width) if all(screen[(row*8+y)*264+x] for y in range(8))]
        visible_cursor = [x for x in full if x-1 in full or x+1 in full]
        cursors.extend([[row, x] for x in visible_cursor])
        chars = []
        for col in range(x0//6, (x0+width)//6):
            key = tuple(screen[(row*8+y)*264+col*6+x] for y in range(8) for x in range(6))
            choices = glyphs.get(key, [])
            # Blank aliases (space and NBSP) have identical display semantics.
            if not any(key):
                chars.append(' ')
            elif len(choices) == 1:
                chars.append(choices[0])
            else:
                chars.append('\ufffd')
                if choices:
                    ambiguous.append({'row': row, 'col': col, 'choices': choices})
                else:
                    unknown.append({'row': row, 'col': col})
        lines.append(''.join(chars).rstrip())
    return Reading('\n'.join(lines).strip('\n'), unknown, ambiguous, cursors,
                   hashlib.sha256(font.read_bytes()).hexdigest())


def stable_read(paths, font=FONT, repeats=2, region=None):
    """Wait through cursor-on frames; require consecutive identical clean captures.

    Never erase a cursor, combine different frames, or guess obscured characters.
    Paths must be an ordered burst captured while no keys are sent.
    """
    last, count = None, 0
    for path in paths:
        reading = decode(path, font, region)
        if reading.clean:
            count = count + 1 if last == reading.text else 1
            last = reading.text
            if count >= repeats:
                return reading, str(path)
        else:
            last, count = None, 0
    raise ValueError('no stable cursor-free text in capture burst (unknown glyph, cursor, or changing screen)')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('screens', type=Path, nargs='+')
    parser.add_argument('--region', nargs=4, type=int, metavar=('X', 'Y', 'WIDTH', 'HEIGHT'))
    parser.add_argument('--font', type=Path, default=FONT)
    parser.add_argument('--burst', action='store_true', help='require two consecutive identical clean frames')
    parser.add_argument('--json', action='store_true')
    parser.add_argument('--expect', action='append', default=[], help='required exact full line, repeatable')
    args = parser.parse_args()
    try:
        if args.burst:
            reading, selected = stable_read(args.screens, args.font, region=args.region)
        else:
            if len(args.screens) != 1:
                parser.error('multiple screenshots require --burst')
            selected = str(args.screens[0])
            reading = decode(selected, args.font, args.region)
        if args.json:
            print(json.dumps(dict(asdict(reading), clean=reading.clean, selected=selected), ensure_ascii=False))
        else:
            print(reading.text)
        if not reading.clean:
            raise ValueError(f'unclean capture: {len(reading.unknown_cells)} unknown, '
                             f'{len(reading.ambiguous_cells)} ambiguous cells; '
                             f'{len(reading.cursor_columns)} cursor columns; capture a burst')
        for line in args.expect:
            if line not in reading.text.splitlines():
                raise ValueError(f'missing exact line: {line!r}')
        return 0
    except (OSError, ValueError) as error:
        print(f'FAIL: {error}', file=sys.stderr)
        return 1


if __name__ == '__main__':
    sys.exit(main())
