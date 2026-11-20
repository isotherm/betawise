#!/usr/bin/env python3
"""Check committed capture hashes and independent AS3000 RAM observations."""
from pathlib import Path
import gzip
import hashlib
import json

HERE = Path(__file__).resolve().parent
manifest = json.loads((HERE / 'evidence/manifest.json').read_text())
for item in manifest['artifacts']:
    data = (HERE / item['path']).read_bytes()
    assert hashlib.sha256(data).hexdigest() == item.get('sha256', item.get('gzip_sha256'))
    if 'raw_sha256' in item:
        raw = gzip.decompress(data)
        assert len(raw) == item['raw_size']
        assert hashlib.sha256(raw).hexdigest() == item['raw_sha256']


def capture(name):
    return gzip.decompress((HERE / 'evidence/as3000-3.01' / (name + '.ram.gz')).read_bytes())


def number(data, address, size):
    return int.from_bytes(data[address:address + size], 'big')


four = capture('four-profiles')
assert four[0x3DF2:0x3DF4] == bytes([1, 4])
for i, name in enumerate(['Alice', 'Bobby', 'Carol', 'David']):
    assert four[0x3D7A + i * 30:0x3D7A + i * 30 + 20].split(b'\0')[0] == name.encode()
for label, profile in [('alice-returned', 1), ('bobby-sees-alice-text', 2)]:
    raw = capture(label)
    assert raw[0x3DF2:0x3DF4] == bytes([1, profile])
    assert number(raw, 0x355C, 4) == 1
    record = number(raw, 0x3442, 4)
    assert record == 0x150A
    assert number(raw, record + 0x44, 2) == 0x0101
    address = number(raw, record, 4)
    length = number(raw, record + 4, 4)
    assert b'PRIVATEALICE' in raw[address:address + length]
log = (HERE / 'evidence/as3000-3.01/mame.log').read_text(errors='replace')
assert 'GROUP4 create=fffffff6' in log
assert 'PASS multi-user qualification complete' in log
assert 'FAIL ' not in log
assert capture('disabled')[0x3DF2] == 0
print('PASS capture hashes, four named profiles, group-4 result, and cross-profile AlphaWord RAM evidence')
