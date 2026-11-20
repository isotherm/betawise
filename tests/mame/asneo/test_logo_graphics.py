"""Graphics oracle tests: bounds alone must not accept damaged or filled shapes."""
import tempfile
import unittest
from pathlib import Path

from PIL import Image, ImageChops, ImageDraw

from logo_graphics import compare, expected_image


class GraphicsTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)

    def capture(self, image, name, left_ink=False):
        full = Image.new('RGB', (264,64), 'white')
        full.paste(ImageChops.invert(image.convert('L')).convert('RGB'), (132,0))
        if left_ink:
            ImageDraw.Draw(full).rectangle((10,10,100,40), fill='black')
        path = self.root/name
        full.save(path)
        return path

    def check_image(self, image, scene='square20'):
        a = self.capture(image, 'a.png')
        b = self.capture(image, 'b.png', left_ink=True)
        return compare([a,b], scene, self.root/'result')

    def test_known_square_geometry_and_left_pane_exclusion(self):
        report = self.check_image(expected_image('square20'))
        self.assertEqual(report['foreground_bounds'], (66,12,87,33))
        self.assertEqual(report['foreground_pixels'], 80)

    def test_missing_edge_pixel_same_bounds_fails(self):
        image = expected_image('square20')
        image.putpixel((70,12), 0)
        with self.assertRaisesRegex(ValueError, '1 wrong pixels'):
            self.check_image(image)
        self.assertTrue((self.root/'result.diff.png').exists())

    def test_filled_square_same_bounds_fails(self):
        image = expected_image('square20')
        ImageDraw.Draw(image).rectangle((66,12,86,32), fill=1)
        with self.assertRaisesRegex(ValueError, '361 wrong pixels'):
            self.check_image(image)

    def test_stray_pixel_outside_shape_fails(self):
        image = expected_image('square20')
        image.putpixel((100,50), 1)
        with self.assertRaises(ValueError): self.check_image(image)

    def test_turtle_and_clear_geometry(self):
        for scene in ['north', 'east', 'blank']:
            report = self.check_image(expected_image(scene), scene)
            self.assertEqual(report['foreground_pixels'], 0 if scene == 'blank' else 24)

    def test_unstable_graphics_fails(self):
        a = self.capture(expected_image('square20'), 'a.png')
        b = self.capture(expected_image('blank'), 'b.png')
        with self.assertRaisesRegex(ValueError, 'changed between captures'):
            compare([a,b], 'square20', self.root/'unstable')
        self.assertTrue((self.root/'unstable.unstable.png').exists())


if __name__ == '__main__':
    unittest.main()
