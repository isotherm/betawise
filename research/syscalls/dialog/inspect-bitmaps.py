#!/usr/bin/env python3
"""Decode original dialog indicator images as text; no SDK symbols or saved assets."""
from pathlib import Path
import hashlib,json,struct
here=Path(__file__).resolve().parent;repo=here.parents[2];workspace=repo.parent
r=next(r for r in json.loads((repo/'research/abi-audit/rom-inventory.json').read_text())if r['name']=='neo_3_21')
b=(workspace/r['path']).read_bytes();assert hashlib.sha256(b).hexdigest()==r['sha256']
for name,address in [('signal',0x44e51c),('scroll both',0x44e374),('scroll up',0x44e384),('scroll down',0x44e394)]:
 width,height,pointer=struct.unpack_from('>HHI',b,address-r['base'])
 data=b[pointer-r['base']:pointer-r['base']+width*((height+7)//8)]
 print(f'{name}: {width} x {height}')
 for y in range(height):print(''.join('#'if data[(y//8)*width+x]>>(y%8)&1 else' 'for x in range(width)))
