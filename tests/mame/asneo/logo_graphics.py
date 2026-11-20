"""Independent Pillow geometry references and exact right-pane comparisons."""
from pathlib import Path

from PIL import Image, ImageChops, ImageDraw

from screen_bitmap import compare as compare_bitmap, reference_image

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


def compare(paths, scene, output):
    """Logo-specific geometry and statistics around the generic bitmap verifier."""
    expected = expected_image(scene)
    reference = REFERENCES/f'{scene}.png'
    stored = reference_image(reference)
    if stored.size != SIZE or stored.tobytes() != expected.tobytes():
        raise ValueError(f'graphics reference disagrees with declared geometry: {reference}')
    report = compare_bitmap(paths, reference, output, region=(132, 0, 132, 64))
    # Retain Logo's shape-only statistics, excluding its fixed pane separator.
    ink = expected.copy()
    ImageDraw.Draw(ink).line((0,0,0,63), fill=0)
    report.update(scene=scene, foreground_bounds=ink.getbbox(),
                  foreground_pixels=sum(bool(pixel) for pixel in ink.getdata()))
    return report


if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--write-references', action='store_true', required=True)
    parser.parse_args()
    write_references()
