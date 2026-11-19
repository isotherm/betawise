#!/usr/bin/env python3
"""Inventory #91 references in original vendor images; no SDK symbol inference."""
from pathlib import Path
import hashlib,json
here=Path(__file__).resolve().parent
workspace=here.parents[3]
shared=here.parents[1]/"abi-audit"
exports=0;references=0
for entry in json.loads((shared/'vendor-image-inventory.json').read_text()):
    b=(workspace/entry['path']).read_bytes()
    assert hashlib.sha256(b).hexdigest()==entry['sha256'],entry['path']
    # Require surrounding syscall words, excluding accidental A16C in resources.
    stubs={i+2 for i in range(0,len(b)-5,2)if b[i:i+6]==bytes.fromhex('a168a16ca170')}
    if not stubs:continue
    exports+=1;hits=set()
    for i in range(0,len(b)-5,2):
        w=int.from_bytes(b[i:i+2],'big')
        if w in (0x4eba,0x6100)or w&0xf1ff==0x41fa:
            target=i+2+int.from_bytes(b[i+2:i+4],'big',signed=True)
            if target in stubs:hits.add((i,target,'PC16 call/address'))
        if w&0xff00==0x6100 and w&255 not in (0,255):
            target=i+2+int.from_bytes(b[i+1:i+2],'big',signed=True)
            if target in stubs:hits.add((i,target,'BSR8'))
        if w==0x4eb9:
            target=int.from_bytes(b[i+2:i+6],'big')
            if target in stubs:hits.add((i,target,'absolute JSR'))
        # Vendor callers commonly load a PC-relative delta into A0 and use
        # JSR/LEA (d8,PC,An.L). Also examine immediate Dn and word forms.
        form=w&0xf1ff
        if form in (0x207c,0x203c,0x307c,0x303c):
            wide=form in (0x207c,0x203c);n=4 if wide else 2
            value=int.from_bytes(b[i+2:i+2+n],'big',signed=True)
            reg=((w>>9)&7)+(8 if form in (0x207c,0x307c)else 0)
            for j in range(i+2+n,min(i+50,len(b)-3),2):
                op=int.from_bytes(b[j:j+2],'big');ext=int.from_bytes(b[j+2:j+4],'big')
                if (op==0x4ebb or op&0xf1ff==0x41fb)and ext>>12==reg:
                    v=value if ext&0x800 else ((value+32768)%65536)-32768
                    target=j+2+v+int.from_bytes(b[j+3:j+4],'big',signed=True)
                    if target in stubs:hits.add((j,target,'immediate-delta indexed-PC candidate'))
    references+=len(hits)
    print(entry['name'],entry['path'],'stubs='+','.join(hex(x)for x in sorted(stubs)),
          'references='+repr(sorted(hits)))
print('Summary:',exports,'images with validated #91 stub;',references,'resolved reference candidates.')
# The AS3000 3.01 LCD map is nearby but not identical to this classification.
r=next(r for r in json.loads((shared/'rom-inventory.json').read_text())if r['name']=='as3000')
b=(workspace/r['path']).read_bytes();assert hashlib.sha256(b).hexdigest()==r['sha256']
bitmap=b[0x2a60c:0x2a62c];mapping=b[0x2a62c:0x2a72c]
accepted=[i for i in range(256)if int.from_bytes(bitmap[(i>>4)*2:(i>>4)*2+2],'big')&(1<<(i&15))]
print('Accepted bytes:',len(accepted))
print('Accepted but LCD map FF:',[hex(i)for i in accepted if mapping[i]==255])
print('Rejected but LCD map not FF:',[hex(i)for i in range(256)if i not in accepted and mapping[i]!=255])
print('Reference search is static; unrecognized dynamic calls are not excluded.')

neo=next(r for r in json.loads((shared/'rom-inventory.json').read_text())if r['name']=='neo_3_21')
nb=(workspace/neo['path']).read_bytes();assert hashlib.sha256(nb).hexdigest()==neo['sha256']
assert bitmap==nb[0x3c4ee:0x3c50e]
print('AS3000 3.01/Neo 3.21 #91 bitmaps are byte-identical; all 256 results match.')
font=nb[0x3a38e:0x3a38e+256*12]  # default ROM font, descriptor 44AF8E
rejected=[i for i in range(256)if i not in accepted]
print('Rejected but default Neo glyph has ink:',sum(any(font[i*12:(i+1)*12])for i in rejected))
print('Rejected blank default Neo glyphs:',[hex(i)for i in rejected if not any(font[i*12:(i+1)*12])])
print('Rejected default Neo fallback-block glyphs:',[hex(i)for i in rejected if font[i*12:(i+1)*12]==font[:12]])
