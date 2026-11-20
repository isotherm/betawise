"""Offline reader contract tests, including failure paths that must never pass OCR."""
import tempfile
import unittest
from pathlib import Path

from PIL import Image

from neo_reader import FONT, decode, stable_read


class ReaderTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)

    def screen(self, text='>>> a=123', cursor=False, scale=1):
        strip = Image.open(FONT).convert('RGB')
        im = Image.new('RGB', (264, 64), 'white')
        for row, line in enumerate(text.splitlines()):
            for col, ch in enumerate(line):
                im.paste(strip.crop((ord(ch)*6, 0, ord(ch)*6+6, 8)), (col*6, row*8))
        if cursor:
            for x in [53, 54]:
                for y in range(8):
                    im.putpixel((x, y), (0, 0, 0))
        im = im.resize((264*scale, 64*scale), Image.Resampling.NEAREST)
        path = self.root/f'{len(list(self.root.iterdir()))}.png'
        im.save(path)
        return path

    def test_real_mame_captures(self):
        fixtures = Path(__file__).parent/'fixtures'
        r = decode(fixtures/'backspace-clean.png')
        self.assertTrue(r.clean)
        self.assertTrue(r.text.endswith('>>> a=124\n>>> a\n124\n>>>'))
        path = fixtures/'backspace-cursor.png'
        self.assertFalse(decode(path).clean)
        with self.assertRaises(ValueError):
            stable_read([path, path])

    def test_exact_and_integer_scale(self):
        for scale in [1, 4]:
            r = decode(self.screen('>>> a=123\n123\n>>>', scale=scale))
            self.assertTrue(r.clean)
            self.assertEqual(r.text, '>>> a=123\n123\n>>>')

    def test_cursor_burst_waits_for_clean_frames(self):
        paths = [self.screen(cursor=True), self.screen(), self.screen()]
        self.assertFalse(decode(paths[0]).clean)
        self.assertTrue(decode(paths[0]).cursor_columns)
        self.assertEqual(stable_read(paths)[1], str(paths[-1]))

    def test_cursor_never_guessed(self):
        with self.assertRaises(ValueError):
            stable_read([self.screen(cursor=True) for _ in range(3)])

    def test_changing_frames_not_combined(self):
        with self.assertRaises(ValueError):
            stable_read([self.screen('one'), self.screen('two')])

    def test_damaged_glyph_rejected(self):
        path = self.screen()
        im = Image.open(path)
        im.putpixel((0, 0), (0, 0, 0))
        im.save(path)
        self.assertFalse(decode(path).clean)

    def test_logo_region_excludes_graphics_and_live_cursor(self):
        path = self.screen('print 42\n42')
        im = Image.open(path)
        for y in range(64):
            for x in range(132, 264):
                if (x*7+y*11) % 5 == 0:
                    im.putpixel((x, y), (0, 0, 0))
        # Logo's manually drawn cursor is confined to the omitted editable row.
        for y in range(56, 64): im.putpixel((12, y), (0, 0, 0))
        im.save(path)
        reading = decode(path, region=(0, 0, 132, 56))
        self.assertTrue(reading.clean)
        self.assertEqual(reading.text, 'print 42\n42')
        self.assertFalse(decode(path).clean)
        # Corruption in the asserted text region must still fail.
        im.putpixel((0, 0), (0, 0, 0)); im.save(path)
        self.assertFalse(decode(path, region=(0, 0, 132, 56)).clean)
        with self.assertRaises(ValueError):
            decode(path, region=(0, 0, 131, 56))

    def test_colour_and_geometry_errors(self):
        for size in [(263, 64), (1056, 256)]:
            path = self.root/'bad.png'
            Image.new('RGB', size, 'white').save(path)
            with self.assertRaises(ValueError):
                decode(path)
        path = self.screen()
        im = Image.open(path)
        im.putpixel((263, 63), (128, 128, 128))
        im.save(path)
        with self.assertRaises(ValueError):
            decode(path)


if __name__ == '__main__':
    unittest.main()
