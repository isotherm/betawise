"""Exact, applet-independent screen bitmap assertions from stable LCD records."""
import argparse
import hashlib
import json
from pathlib import Path

from PIL import Image, ImageChops

from neo_reader import bitmap, is_lcd, lcd_frames


def reference_image(path):
    """References use black foreground on white; reject fuzzy/antialiased pixels."""
    with Image.open(path) as source:
        image = source.convert('RGB')
    if any(pixel not in ((0, 0, 0), (255, 255, 255)) for pixel in image.getdata()):
        raise ValueError(f'reference must contain only black and white pixels: {path}')
    return image.convert('L').point(lambda value: int(value == 0), mode='1')


def capture_images(paths, region=None, screen_size=(264, 64)):
    """Return logical ink images; one RAM record already proves three equal samples."""
    paths = list(paths)
    if not paths or (len(paths) < 2 and not is_lcd(paths[0])):
        raise ValueError('bitmap check needs a stable LCD record or two captures')
    width, height = screen_size
    region = tuple(region) if region is not None else (0, 0, width, height)
    if (len(region) != 4 or any(type(v) is not int for v in region)
            or min(region[:2]) < 0 or min(region[2:]) <= 0
            or region[0]+region[2] > width or region[1]+region[3] > height):
        raise ValueError('bitmap region must lie within the screen')
    x, y, w, h = region
    images = []
    for path in paths:
        if is_lcd(path):
            if screen_size != (264, 64):
                raise ValueError('Neo LCD records require a 264x64 screen')
            frames = lcd_frames(path)
        else:
            frames = [bitmap(path, screen_size)]
        for bits in frames:
            image = Image.new('1', (w, h))
            image.putdata([bits[yy*width+xx] for yy in range(y, y+h) for xx in range(x, x+w)])
            images.append(image)
    return images, region


def compare(paths, reference, output, region=None, screen_size=(264, 64)):
    """Compare every selected pixel with a reviewed PNG; never update the reference."""
    reference, output = Path(reference), Path(output)
    artifacts = [output.with_suffix(suffix) for suffix in ('.actual.png', '.expected.png', '.diff.png', '.unstable.png', '.json')]
    if reference.resolve() in [path.resolve() for path in artifacts]:
        raise ValueError('diagnostic output must not overwrite the reference')
    expected = reference_image(reference)
    images, region = capture_images(paths, region, screen_size)
    if expected.size != images[0].size:
        raise ValueError(f'reference size {expected.size} does not match region {images[0].size}')
    output.parent.mkdir(parents=True, exist_ok=True)
    for image in images[1:]:
        if image.tobytes() != images[0].tobytes():
            diff_path = output.with_suffix('.unstable.png')
            ImageChops.logical_xor(images[0], image).convert('L').save(diff_path)
            raise ValueError(f'bitmap changed between captures; diff: {diff_path}')
    actual = images[0]
    difference = ImageChops.logical_xor(actual, expected)
    mismatch = sum(bool(pixel) for pixel in difference.getdata())
    ImageChops.invert(actual.convert('L')).save(output.with_suffix('.actual.png'))
    ImageChops.invert(expected.convert('L')).save(output.with_suffix('.expected.png'))
    if mismatch:
        diff_path = output.with_suffix('.diff.png')
        difference.convert('L').save(diff_path)
        raise ValueError(f'{mismatch} wrong pixels; diff: {diff_path}')
    return dict(region=list(region), foreground_bounds=actual.getbbox(),
                foreground_pixels=sum(bool(pixel) for pixel in actual.getdata()),
                differing_pixels=0, reference=str(reference),
                reference_sha256=hashlib.sha256(reference.read_bytes()).hexdigest())


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('captures', type=Path, nargs='+')
    parser.add_argument('--reference', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True, help='prefix for diagnostic PNGs and JSON result')
    parser.add_argument('--region', type=int, nargs=4, metavar=('X', 'Y', 'WIDTH', 'HEIGHT'))
    args = parser.parse_args()
    try:
        result = compare(args.captures, args.reference, args.output, args.region)
        args.output.with_suffix('.json').write_text(json.dumps(result, indent=2)+'\n')
        print('PASS exact screen bitmap')
        return 0
    except (OSError, ValueError) as error:
        parser.exit(1, f'FAIL: {error}\n')


if __name__ == '__main__':
    raise SystemExit(main())
