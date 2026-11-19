#!/usr/bin/env python3
"""Run read-only ABI audit against local private firmware, in disposable state."""
from pathlib import Path
import os, subprocess, sys, tempfile, json, hashlib
here=Path(__file__).resolve().parent
repo=here.parents[1]
workspace=repo.parent
sys.path.insert(0, str(here.parent / 'syscalls/coverage'))
from syscall_coverage import instrument
out=Path(tempfile.mkdtemp(prefix='os3k-abi-')) if len(sys.argv)<2 else Path(sys.argv[1]).resolve()
out.mkdir(parents=True,exist_ok=True)
flags=['-m68000','-msoft-float','-msep-data','-fomit-frame-pointer','-fshort-enums','-fno-store-merging','-Os','-fno-builtin',f'-I{repo / "os3k"}']
def cmd(args,**kw):subprocess.run(list(map(str,args)),check=True,**kw)
# Check current production adapters as well as the original ROM conventions.
cmd([sys.executable,repo/'tests/qemu/abi-bridges.py'])
for r in json.loads((here/'rom-inventory.json').read_text()):
 assert hashlib.sha256((workspace/r['path']).read_bytes()).hexdigest()==r['sha256'],r['name']+' ROM hash differs'
cmd(['m68k-linux-gnu-gcc',*flags,'-S',here/'compiler-probe.c','-o',out/'compiler.s'])
cmd(['m68k-linux-gnu-gcc',*flags,'-c',here/'typed-probe.c','-o',out/'typed.o'])
cmd(['m68k-linux-gnu-ld','-Ttext=0x36000','-e','typed_probe',out/'typed.o','-o',out/'typed.elf'])
cmd(['m68k-linux-gnu-objcopy','-O','binary',out/'typed.elf',out/'typed.bin'])
cmd(['m68k-linux-gnu-as','-m68000',here/'bridge-probe.S','-o',out/'bridge.o'])
cmd(['m68k-linux-gnu-objcopy','-O','binary',out/'bridge.o',out/'bridge.bin'])
cmd([sys.executable,here/'make-pointer-probe.py',out])
cmd([sys.executable,here.parent/'syscalls'/'serial'/'make-serial-probe.py',out])
cmd([sys.executable,here.parent/'syscalls'/'localization'/'make-localization-probe.py',out])
cmd([sys.executable,here.parent/'syscalls'/'power'/'make-battery-dialog-probe.py',out])
for name in ['typed','varargs']:
 (out/(name+'.lua')).write_text((here/(name+'-probe.lua')).read_text())
roms=out/'roms'/'asneo';roms.mkdir(parents=True,exist_ok=True)
for filename in ['os3kneorom.os3kos','smallos3kneorom.os3kos']:
 link=roms/filename
 if not link.exists():link.symlink_to(workspace/'Ghidra'/'Alphasmart Neo'/'SmartApplets'/filename)
def mame(name,neo,script,marker,seconds=12):
 folder=out/name;folder.mkdir(exist_ok=True)
 exe=workspace/'mame'/('neodiag' if neo else 'mame')
 observed = folder / 'observed.lua'
 coverage_first = (120 if neo else 530) if any(
  part in name for part in ['serial', 'localization', 'battery-dialog']) else 180
 observed.write_text(instrument((out / script).read_text(), neo, coverage_first))
 args=[exe,'asneo' if neo else 'asma3k','-rompath',out/'roms' if neo else workspace/'mame'/'roms','-nvram_directory',folder/'nvram','-cfg_directory',folder/'cfg','-video','none','-sound','none','-nothrottle','-seconds_to_run',str(seconds),'-autoboot_delay','0','-autoboot_script',observed]
 with (folder/'mame.log').open('w')as log:cmd(args,cwd=out,env={**os.environ,'SDL_VIDEODRIVER':'dummy'},stdout=log,stderr=subprocess.STDOUT)
 text=(folder/'mame.log').read_text()
 assert marker in text and 'LUA ERROR' not in text and 'FAIL ABI' not in text,name+': inspect '+str(folder/'mame.log')
 print('PASS',name)
for neo,name in [(False,'as3000'),(True,'neo_3_21')]:
 mame(name,neo,name+'.lua','PASS ABI pointer probe 35 cases')
 mame(name+'-varargs',neo,'varargs.lua','DONE variadic characterization')
 text=(out/(name+'-varargs')/'mame.log').read_text()
 for expected in ['text=305419896 305419896 12345678','text=1.5','text=ld 11223344','output=3fc00000 a5a5a5a5','output=3ff80000 00000000','D0=0 text=1234567890123 output=a5a5a5a5']:
  assert expected in text,expected
mame('neo-typed',True,'typed.lua','PASS aggregate: writes six bytes through A1; A0=result+4; boundary sentinel preserved')
for neo,name in [(False,'as'),(True,'neo')]:
 mame(name+'-serial',neo,'serial-'+name+'.lua','PASS ABI serial probe')
for neo,name in [(False,'as'),(True,'neo')]:
 mame(name+'-localization',neo,'localization-'+name+'.lua','PASS ABI localization/power probe',24)
for neo,name in [(False,'as'),(True,'neo')]:
 mame(name+'-battery-dialog',neo,'battery-dialog-'+name+'.lua','PASS ABI battery/dialog probe',28)
print('Evidence:',out)
