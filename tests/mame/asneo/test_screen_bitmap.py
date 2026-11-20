"""Applet-independent bitmap assertions, including empty screens and arbitrary regions."""
import json
from pathlib import Path
import tempfile
import unittest

from PIL import Image

from screen_bitmap import compare


class ScreenBitmapTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.capture = self.root/'screen.lcd.json'
        controllers = [dict(ram=bytes(1188).hex(), m_start_line=0, m_display_on=1,
                            m_adc=0, m_com_reverse=0, m_reverse=0, m_all_points=0,
                            m_reset=0) for _ in range(2)]
        # An isolated pixel in the left controller, unrelated to Logo geometry.
        ram = bytearray(1188); ram[132+17] = 1 << 3  # x=17, y=11
        controllers[0]['ram'] = ram.hex()
        self.record = dict(format='neo-lcd-v1', samples=[dict(controllers=controllers)]*3)
        self.capture.write_text(json.dumps(self.record))

    def reference(self, size, pixel=None):
        image = Image.new('1', size, 1)
        if pixel is not None: image.putpixel(pixel, 0)
        path = self.root/'reference.png'; image.save(path)
        return path

    def test_full_screen(self):
        report = compare([self.capture], self.reference((264,64), (17,11)), self.root/'full')
        self.assertEqual(report['foreground_pixels'], 1)
        self.assertEqual(report['foreground_bounds'], (17,11,18,12))
        self.assertEqual(report['region'], [0,0,264,64])

    def test_arbitrary_region_and_blank_region(self):
        report = compare([self.capture], self.reference((9,7), (2,3)), self.root/'crop', (15,8,9,7))
        self.assertEqual(report['foreground_bounds'], (2,3,3,4))
        report = compare([self.capture], self.reference((30,20)), self.root/'blank', (200,40,30,20))
        self.assertEqual(report['foreground_pixels'], 0)
        self.assertIsNone(report['foreground_bounds'])

    def test_missing_pixel_fails_and_preserves_reference(self):
        reference = self.reference((264,64)); original = reference.read_bytes()
        with self.assertRaisesRegex(ValueError, '1 wrong pixels'):
            compare([self.capture], reference, self.root/'bad')
        self.assertEqual(reference.read_bytes(), original)
        self.assertTrue((self.root/'bad.diff.png').exists())

    def test_bad_geometry_and_fuzzy_reference_fail(self):
        reference = self.reference((9,7))
        for region in [(260,0,9,7), (-1,0,9,7), (0,0,0,7)]:
            with self.assertRaises(ValueError): compare([self.capture], reference, self.root/'bad', region)
        with self.assertRaisesRegex(ValueError, 'reference size'):
            compare([self.capture], reference, self.root/'bad')
        Image.new('L', (264,64), 128).save(reference)
        with self.assertRaisesRegex(ValueError, 'only black and white'):
            compare([self.capture], reference, self.root/'bad')


if __name__ == '__main__':
    unittest.main()
