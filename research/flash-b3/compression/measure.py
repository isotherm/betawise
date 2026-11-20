#!/usr/bin/env python3
"""Host-only sizing experiment, not an embedded compressor or document corpus."""
import hashlib
import importlib.metadata
import json
from pathlib import Path
import random
import struct

import lz4.block

ROOT = Path(__file__).resolve().parents[3]
BLOCK = 8192
RECORD_HEADER = 64
BLOCK_HEADER = 16

def encoded_size(data):
    size = RECORD_HEADER
    for start in range(0, len(data), BLOCK):
        raw = data[start:start+BLOCK]
        compressed = lz4.block.compress(raw, store_size=False)
        assert lz4.block.decompress(compressed, uncompressed_size=len(raw)) == raw
        # Store raw if compression fails to reduce the payload.
        size += BLOCK_HEADER + min(len(raw), len(compressed))
    return size

def splice(base, current):
    prefix = 0
    while prefix < min(len(base), len(current)) and base[prefix] == current[prefix]:
        prefix += 1
    suffix = 0
    while suffix < min(len(base), len(current))-prefix and base[-suffix-1] == current[-suffix-1]:
        suffix += 1
    removed = len(base)-prefix-suffix
    inserted = current[prefix:len(current)-suffix if suffix else len(current)]
    recipe = struct.pack('>III', prefix, removed, len(inserted)) + inserted
    p, d, n = struct.unpack('>III', recipe[:12])
    assert len(recipe) == 12+n
    assert base[:p] + recipe[12:] + base[p+d:] == current
    return recipe

files = [ROOT/'README.md', ROOT/'applets/FileManager/README.md', ROOT.parent/'neotools/README.md']
rows = []
for file in files:
    data = file.read_bytes()
    rows.append({'file': str(file.relative_to(ROOT.parent)), 'bytes': len(data),
        'sha256': hashlib.sha256(data).hexdigest(), 'encoded_bytes': encoded_size(data)})

base = (ROOT/'applets/FileManager/README.md').read_bytes()
histories = {}
for name in ('append', 'local-replacement'):
    versions = [base]
    for number in range(1, 21):
        if name == 'append':
            versions.append(versions[-1] + f'\nRevision {number:02}: add one short paragraph to the document.\n'.encode())
        else:
            position = len(base)//2
            replacement = f'revision-{number:02}-text'.encode()
            versions.append(base[:position] + replacement + base[position+len(replacement):])
    cost = encoded_size(versions[0])
    deltas = 0
    chain = 0
    for previous, current in zip(versions, versions[1:]):
        full = encoded_size(current)
        delta = encoded_size(splice(previous, current))
        if chain < 7 and delta < full:
            cost += delta
            deltas += 1
            chain += 1
        else:
            cost += full
            chain = 0
    histories[name] = {'versions': len(versions), 'raw_payload_total': sum(map(len, versions)),
        'compressed_full_total': sum(map(encoded_size, versions)),
        'bounded_delta_total': cost, 'delta_records': deltas, 'maximum_delta_chain': 7}

random_data = random.Random(68000).randbytes(32768)
print(json.dumps({'host_lz4_version': importlib.metadata.version('lz4'), 'block_bytes': BLOCK,
    'assumed_record_overhead': RECORD_HEADER, 'assumed_block_overhead': BLOCK_HEADER,
    'documents': rows, 'synthetic_histories': histories,
    'incompressible_control': {'bytes': len(random_data), 'encoded_bytes': encoded_size(random_data)}}, indent=2))
