#!/usr/bin/env python3
"""Cold-boot traces on disposable flash; not a flash programmer/recovery engine."""
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
WS = ROOT.parent
OUT = Path(sys.argv[1]).resolve()
OUT.mkdir(parents=True, exist_ok=False)
manifest = []
for model in ('neo13', 'neo13small', 'neo05', 'as3000'):
    if len(sys.argv)>2 and model not in sys.argv[2:]: continue
    for patched in (False, True):
        name = model + ('-patched' if patched else '-stock')
        run = OUT/name
        driver = 'asneo' if model.startswith('neo') else 'asma3k'
        roms = run/'roms'/driver
        nvram = run/'nvram'/driver
        roms.mkdir(parents=True)
        nvram.mkdir(parents=True)
        if model in ('neo13','neo13small'):
            firmware = WS/'Ghidra/Alphasmart Neo/SmartApplets'
            small = (firmware/'smallos3kneorom.os3kos').read_bytes()
            system = (firmware/'os3kneorom.os3kos').read_bytes()
            assert hashlib.sha256(system).hexdigest() == '32fe55bb50c1b58326bd303f14973b8402ddf8bf6662b210e4d5e9f931c7e6c0'
            flash = bytearray(b'\xff'*0x400000)
            flash[:len(small)] = small
            source = 0
            for pos in range(0x50,0x70,8):
                addr = int.from_bytes(system[pos:pos+4],'big')
                size = int.from_bytes(system[pos+4:pos+8],'big')
                if not addr and not size: break
                size = (size+1023)&~1023
                flash[addr-0x400000:addr-0x400000+size] = system[source:source+size]
                source += size
            applet = (firmware/'alphawordplus.os3kapp').read_bytes()
            flash[0x70000:0x70000+len(applet)] = applet
            stub, original, patch = 0x4580FC, 0x4500FC, 0x450182
            assert flash[patch-0x400000] == 0xFF
            config = run/'cfg'
            config.mkdir()
            (config/'asneo.cfg').write_text('<mameconfig version="10"><system name="asneo"><input><port tag=":NOR_PROFILE" type="CONFIG" mask="1" defvalue="0" value="1" /></input></system></mameconfig>')
            (roms/'smallos3kneorom.os3kos').write_bytes(small)
            (roms/'os3kneorom.os3kos').write_bytes(system)
            sites = [('candidate',0x450180),('scan',0x4130FC)]
            if model == 'neo13small':
                stub, original, patch = 0x46FF18, 0x46FF98, 0x401443
                assert flash[patch-0x400000] == 0x98
                sites = [('candidate',0x40143E),('original',0x410082),('scan',0x4130FC)]
            executable = WS/'mame/neodiag'
            flash_name = 'flashnv'
        elif model == 'neo05':
            firmware = WS/'Ghidra/Alphasmart 3000/SmartApplets'
            small = (firmware/'SMALLOS3KNeoROM.OS3KOS').read_bytes()
            system = (firmware/'OS3KNeoROM.OS3KOS').read_bytes()
            assert hashlib.sha256(system).hexdigest() == '5f550e48ad36892ee5f5a067e8c1b87f6bf504b6cb2c2dfc4098776fa8831a3d'
            flash = bytearray(b'\xff'*0x400000)
            flash[:len(small)] = small
            flash[0x1C0000:0x200000] = system[:0x40000]
            flash[0x6000:0x6014] = system[0x40000:0x40014]
            applet = (firmware/'AlphaWordPlus.OS3KApp').read_bytes()
            flash[0x8000:0x8000+len(applet)] = applet
            stub, original, patch = 0x586BD0, 0x5C6BD0, 0x5C0089
            assert flash[patch-0x400000] == 0x5C
            config = run/'cfg'
            config.mkdir()
            (config/'asneo.cfg').write_text('<mameconfig version="10"><system name="asneo"><input><port tag=":NOR_PROFILE" type="CONFIG" mask="1" defvalue="0" value="1" /></input></system></mameconfig>')
            # Loader supports 2013 packages; complete disposable NVRAM above
            # supplies the actual 2005 CPU-visible firmware for this experiment.
            loader = WS/'Ghidra/Alphasmart Neo/SmartApplets'
            for filename in ('smallos3kneorom.os3kos','os3kneorom.os3kos'):
                shutil.copyfile(loader/filename,roms/filename)
            sites = [('candidate',0x5C0086),('original',original)]
            executable = WS/'mame/neodiag'
            flash_name = 'flashnv'
        else:
            firmware = WS/'Ghidra/Alphasmart 3000/SmartApplets'
            small = (firmware/'SMALLOS3KROM.OS3KOS').read_bytes()
            system = (firmware/'OS3KROM.OS3KOS').read_bytes()
            assert hashlib.sha256(system).hexdigest() == '732dca63399d883071ce41cc07ad352fa19c506b7b8c307ace16d5c53e6f8708'
            flash = bytearray(b'\xff'*0x100000)
            flash[:len(small)] = small
            flash[0xC0000:0x100000] = system[:0x40000]
            flash[0x4000:0x7BF0] = system[0x40000:0x43BF0]
            applet = (WS/'mame/roms/asma3k/alphawordplus.os3kapp').read_bytes()
            flash[0x8000:0x8000+len(applet)] = applet
            stub, original, patch = 0x487390, 0x4C7390, 0x4C0089
            assert flash[patch-0x400000] == 0x4C
            (roms/'smallos3krom.os3kos').write_bytes(small)
            (roms/'os3krom.os3kos').write_bytes(system)
            config = run/'cfg'
            config.mkdir()
            sites = [('candidate',0x4C0086),('original',original)]
            executable = WS/'mame/mame'
            flash_name = 'flash'
        if patched:
            off = stub-0x400000
            assert flash[off:off+16] == b'\xff'*16
            flash[off:off+16] = bytes.fromhex('23fc52454356000300004ef9')+original.to_bytes(4,'big')
            flash[patch-0x400000] &= 0x7F if model.startswith('neo13') else 0xFB
        if model == 'as3000':
            # AS3000 share is serialized as native uint16_t words on this host.
            swapped = bytearray(len(flash))
            swapped[0::2],swapped[1::2] = flash[1::2],flash[0::2]
            flash = swapped
        (nvram/flash_name).write_bytes(flash)
        manifest.append(dict(case=name,system_sha256=hashlib.sha256(system).hexdigest(),small_sha256=hashlib.sha256(small).hexdigest(),flash_sha256=hashlib.sha256(flash).hexdigest()))
        definitions = f"local isneo={str(model.startswith('neo')).lower()}\nlocal stub=0x{stub:X}\nlocal sites={{"+','.join('{"'+n+'",0x'+f'{a:X}'+'}' for n,a in sites)+'}\n'
        script = run/'probe.lua'
        script.write_text(definitions+(HERE/'probe.lua').read_text())
        command = [str(executable),driver,'-rompath',str(run/'roms')+';'+str(WS/'mame/roms'),'-nvram_directory',str(run/'nvram'),'-cfg_directory',str(config),'-video','none','-sound','none','-nothrottle','-skip_gameinfo','-autoboot_delay','0','-autoboot_script',str(script),'-seconds_to_run','18']
        (OUT/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
        with (run/'mame.log').open('w') as log:
            result = subprocess.run(command,cwd=run,env={**os.environ,'SDL_VIDEODRIVER':'dummy'},stdout=log,stderr=subprocess.STDOUT,timeout=120)
        text = (run/'mame.log').read_text()
        print(name,flush=True)
        print('\n'.join(line for line in text.splitlines() if line.startswith(('EVENT','FINAL','SEEN','PROBE_COMPLETE'))),flush=True)
        assert result.returncode == 0 and 'PROBE_COMPLETE' in text and 'LUA ERROR' not in text, text
        if model == 'neo13':
            assert 'SEEN scan true' in text and 'SEEN candidate false' in text and 'SEEN recovery false' in text, text
            print('PASS rejected as universal boot hook',flush=True)
        else:
            assert 'SEEN candidate true' in text, text
            if model == 'neo13small':
                assert 'SEEN scan true' in text, text
                if patched: assert text.index(' recovery PC=') < text.index(' scan PC='), text
            assert ('SEEN recovery true' in text) == patched, text
            print('PASS cold-boot trampoline control flow',flush=True)
(OUT/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
