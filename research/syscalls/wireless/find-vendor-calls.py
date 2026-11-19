#!/usr/bin/env python3
"""Inventory slots 237–277 references in original vendor images; no SDK symbol inference."""
from pathlib import Path
import hashlib,json
here=Path(__file__).resolve().parent
workspace=here.parents[3]
shared=here.parents[1]/"abi-audit"
exports=0;references=0
for entry in json.loads((shared/'vendor-image-inventory.json').read_text()):
    b=(workspace/entry['path']).read_bytes()
    assert hashlib.sha256(b).hexdigest()==entry['sha256'],entry['path']
    # Require surrounding syscall words, excluding accidental trap words in resources.
    stubs={}
    for i in range(0,len(b)-5,2):
        if b[i:i+6]==bytes.fromhex('a3b4a3b8a3bc'):
            for slot in range(237,278):
                off=i+2*(slot-237)
                if b[off:off+2]==(0xa000+4*slot).to_bytes(2,'big'):stubs[off]=slot
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
            for j in range(i+2+n,min(i+2+n+2,len(b)-3),2):
                op=int.from_bytes(b[j:j+2],'big');ext=int.from_bytes(b[j+2:j+4],'big')
                if (op==0x4ebb or op&0xf1ff==0x41fb)and ext>>12==reg:
                    v=value if ext&0x800 else ((value+32768)%65536)-32768
                    target=j+2+v+int.from_bytes(b[j+3:j+4],'big',signed=True)
                    if target in stubs:hits.add((j,target,'immediate-delta indexed-PC candidate'))
    references+=len(hits)
    print(entry['name'],entry['path'],'references='+repr([(hex(x),stubs[y],t)for x,y,t in sorted(hits)]))
print('Summary:',exports,'images with validated syscall block;',references,'resolved reference candidates.')
