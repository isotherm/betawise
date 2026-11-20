#!/usr/bin/env python3
"""Test forced updater reachability with an unusable NEO13 handoff pointer."""
import os
from pathlib import Path
import shutil
import subprocess
import sys

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[3]
OUT=Path(sys.argv[1]).resolve()
OUT.mkdir(parents=True,exist_ok=False)
# Reuse an independently prepared stock fixture, never a live session.
seed=Path(sys.argv[2]).resolve()
for name,broken,force in [('stock-forced',False,True),('broken-forced',True,True),('broken-normal',True,False)]:
    if len(sys.argv)>3 and name not in sys.argv[3:]: continue
    out=OUT/name
    (out/'nvram/asneo').mkdir(parents=True)
    shutil.copytree(seed/'roms',out/'roms')
    shutil.copytree(seed/'cfg',out/'cfg')
    flash=bytearray((seed/'nvram/asneo/flashnv').read_bytes())
    assert flash[0x143E:0x1444]==bytes.fromhex('4EB90046FF98')
    if broken:
        old=int.from_bytes(flash[0x1440:0x1444],'big')
        assert old&0x405F98==0x405F98
        flash[0x1440:0x1444]=(0x405F98).to_bytes(4,'big')
        assert flash[0x5F98:0x5FA8]==b'\xff'*16
    (out/'nvram/asneo/flashnv').write_bytes(flash)
    script=out/'probe.lua'
    script.write_text('local force='+str(force).lower()+'\n'+(HERE/'probe.lua').read_text())
    with (out/'mame.log').open('w') as log:
        result=subprocess.run([str(ROOT.parent/'mame/neodiag'),'asneo','-rompath',str(out/'roms'),'-nvram_directory',str(out/'nvram'),'-cfg_directory',str(out/'cfg'),'-video','none','-sound','none','-nothrottle','-skip_gameinfo','-autoboot_delay','0','-autoboot_script',str(script),'-seconds_to_run','45'],cwd=out,env={**os.environ,'SDL_VIDEODRIVER':'dummy'},stdout=log,stderr=subprocess.STDOUT,timeout=120)
    text=(out/'mame.log').read_text()
    print(name,flush=True)
    print('\n'.join(line for line in text.splitlines() if line.startswith(('REACHED','FINAL','SEEN','FALLBACK_COMPLETE'))),flush=True)
    assert result.returncode==0 and 'FALLBACK_COMPLETE' in text and 'LUA ERROR' not in text,text
