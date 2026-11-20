"""Independent Pillow geometry references and exact right-pane comparisons."""
import hashlib
from pathlib import Path

from PIL import Image, ImageChops, ImageDraw

from neo_reader import bitmap

SIZE = (132, 64)
REFERENCES = Path(__file__).with_name('fixtures') / 'logo-graphics'
# Coordinates relative to the right pane. Home is (66,32); Logo zero heads north.
# The LCD turtle is a 3-pixel half-base and 10-pixel nose at cardinal headings.
NORTH = [(69,32), (63,32), (66,22)]
EAST = [(66,35), (66,29), (76,32)]
SCENES = {
    'blank': {},
    'north': {'turtle': NORTH},
    'east': {'turtle': EAST},
    'north-moved': {'turtle': [(69,22), (63,22), (66,12)]},
    'square20': {'rectangle': (66,12,86,32)},
    'square20-north': {'rectangle': (66,12,86,32), 'turtle': NORTH},
    'pen-line': {'lines': [((66,22),(81,22))]},
    'home-line': {'lines': [((66,22),(81,22)), ((76,22),(66,32))]},
    'square10': {'rectangle': (66,22,76,32)},
}


def expected_image(scene):
    spec = SCENES[scene]
    image = Image.new('1', SIZE, 0)  # logical 1 means black LCD pixel
    draw = ImageDraw.Draw(image)
    draw.line((0,0,0,63), fill=1)  # fixed pane separator
    if 'rectangle' in spec:
        draw.rectangle(spec['rectangle'], outline=1)
    for start, end in spec.get('lines', []):
        draw.line([start, end], fill=1)
    vertices = spec.get('turtle', [])
    for start, end in zip(vertices, vertices[1:]+vertices[:1]):
        edge = Image.new('1', SIZE, 0)
        ImageDraw.Draw(edge).line([start, end], fill=1)
        edge.putpixel(end, 0)  # half-open edges invert each vertex once
        image = ImageChops.logical_xor(image, edge)
    return image


def write_references():
    """Explicit authoring operation; never called by test/verification paths."""
    REFERENCES.mkdir(parents=True, exist_ok=True)
    for scene in SCENES:
        # Store conventional black foreground on white, rather than logical bits.
        ImageChops.invert(expected_image(scene).convert('L')).save(REFERENCES/f'{scene}.png')


def pane(path):
    bits = bitmap(path, (264,64))
    image = Image.new('1', SIZE)
    image.putdata([bits[y*264+x] for y in range(64) for x in range(132,264)])
    return image


def compare(paths, scene, output):
    """Require two identical captures, then compare every pixel with fixed geometry."""
    paths = list(paths)
    if len(paths) < 2:
        raise ValueError('graphics check needs two captures')
    expected = expected_image(scene)
    reference = REFERENCES/f'{scene}.png'
    with Image.open(reference) as stored:
        stored_bits = stored.convert('L').point(lambda p: int(p < 128), mode='1')
    if stored_bits.size != SIZE or stored_bits.tobytes() != expected.tobytes():
        raise ValueError(f'graphics reference disagrees with declared geometry: {reference}')
    images = [pane(path) for path in paths]
    for image in images[1:]:
        if image.tobytes() != images[0].tobytes():
            output = Path(output)
            output.parent.mkdir(parents=True, exist_ok=True)
            diff_path = output.with_suffix('.unstable.png')
            ImageChops.logical_xor(images[0], image).convert('L').save(diff_path)
            raise ValueError(f'{scene}: graphics changed between captures; diff: {diff_path}')
    actual = images[0]
    difference = ImageChops.logical_xor(actual, expected)
    mismatch = sum(bool(p) for p in difference.getdata())
    output = Path(output)
    output.parent.mkdir(parents=True, exist_ok=True)
    ImageChops.invert(actual.convert('L')).save(output.with_suffix('.actual.png'))
    ImageChops.invert(expected.convert('L')).save(output.with_suffix('.expected.png'))
    if mismatch:
        difference.convert('L').save(output.with_suffix('.diff.png'))
        raise ValueError(f'{scene}: {mismatch} wrong pixels; diff: {output.with_suffix(".diff.png")}')
    ink = actual.copy()
    ImageDraw.Draw(ink).line((0,0,0,63), fill=0)
    return dict(scene=scene, region=[132,0,132,64], foreground_bounds=ink.getbbox(),
                foreground_pixels=sum(bool(p) for p in ink.getdata()), differing_pixels=0,
                reference=str(reference), reference_sha256=hashlib.sha256(reference.read_bytes()).hexdigest())


if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--write-references', action='store_true', required=True)
    parser.parse_args()
    write_references()
