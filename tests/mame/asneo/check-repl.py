#!/usr/bin/env python3
"""Offline Neo REPL parity checks and Logo transcript assertions, using native keys."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

from neo_reader import stable_read
from logo_graphics import compare as compare_graphics, REFERENCES
from check import HERE, ROOT, build_images


class Scenario:
    def __init__(self, applet='micropython'):
        self.lua = [f'boot({json.dumps(applet)})']
        self.checks = []
        self.region = [0, 0, 132, 56] if applet == 'logo' else None

    def text(self, text):
        self.lua.append(f'type_text({json.dumps(text)})')

    def key(self, key, modifier=None, hold=None):
        self.lua.append(f'key({json.dumps(key)}, {json.dumps(modifier) if modifier else "nil"}, {hold or "nil"})')

    def ctrl(self, key, hold=None):
        self.key(key, 'Left Ctrl', hold=hold)
        self.wait(90)

    def wait(self, frames=150):
        self.lua.append(f'wait({frames})')

    def check(self, name, lines=(), suffix=None, contains=(), absent=(), region=None, equals=None):
        self.lua.append('checkpoint()')
        self.checks.append(dict(name=name, lines=list(lines), suffix=suffix,
                                contains=list(contains), absent=list(absent), region=region or self.region, equals=equals))

    def graphics(self, name, scene):
        self.lua.append('checkpoint()')
        self.checks.append(dict(name=name, graphics=scene))

    def enter(self, command, expected=None, name=None):
        self.text(command+'\r')
        self.wait()
        if expected:
            self.check(name or expected, lines=[expected])


def scenario(name, variant):
    s = Scenario('logo' if name.startswith('logo') else 'micropython')
    if name == 'editing':
        s.text('print("BACKSPACE",123)X'); s.key('Backspace'); s.enter('', 'BACKSPACE 123')
        s.text('x'); s.key('Backspace'); s.key('Backspace')
        s.check('empty Backspace', suffix='>>>', absent=['>>> >>>'])
        s.text('x'); s.key('Home'); s.ctrl('D')
        s.check('delete to empty', suffix='>>>', absent=['>>> >>>'])
        s.enter('print("DELETE",1)', 'DELETE 1')
        s.text('discard this draft'); s.ctrl('U'); s.enter('print("CTRLU",1)', 'CTRLU 1')
        s.text('pint("CTRLF",1)'); s.key('Home'); s.ctrl('F'); s.text('r'); s.enter('', 'CTRLF 1')
        s.text('print("CTRLK",1)junk')
        for _ in range(4): s.key('Left')
        s.ctrl('K'); s.enter('', 'CTRLK 1')
        s.text('print("EDIT",12)'); s.key('Left'); s.key('Left'); s.text('0'); s.enter('', 'EDIT 102')
    elif name == 'terminal':
        # Keep the prompt outside each asserted region. Every operation is
        # emitted by Python through the production HAL, not injected pixels.
        s.enter('t=lambda s: print(s.replace("@",chr(27)),end="")')
        def screen(command, label, expected, region):
            s.enter(command)
            s.check(label, equals=expected, region=region)
        screen('t("@[2J@[HVT100@[6;1H")', 'clear and home', 'VT100', [0,0,264,40])
        screen('print(chr(27)+"[3;",end="");t("5HSPLIT@[6;1H")',
               'escape survives separate output calls', '    SPLIT', [0,16,264,8])
        screen('t("@[2J@[3;5HABCDE@[2D!@[6;1H")',
               'relative cursor movement', '    ABC!E', [0,16,264,8])
        screen('t("@[2J@[999;999H@[999A@[999DCLAMP@[6;1H")',
               'cursor clamps to screen', 'CLAMP', [0,0,264,40])
        for mode, expected in [(0, '    AB'), (1, '      CD'), (2, '')]:
            screen(f't("@[2J@[3;5HABCD@[3;{7 if mode == 0 else 6}H@[{mode}K@[6;1H")',
                   f'erase line mode {mode}', expected, [0,16,264,8])
        screen('t("@[2J@[2;1HABOVE@[3;1HABCD@[4;1HBELOW@[3;2H@[J@[6;1H")',
               'erase display below cursor', 'ABOVE\nA', [0,8,264,24])
        screen('t("@[2J@[2;1HABOVE@[3;1HABCD@[4;1HBELOW@[3;2H@[1J@[6;1H")',
               'erase display above cursor preserves below', '  CD\nBELOW', [0,16,264,16])
        s.check('erase display above cursor clears earlier rows', equals='', region=[0,0,264,16])
        s.enter('print("SCROLL"+chr(10),end="")')
        s.enter('print(("SCROLL"+chr(10))*10,end="")')
        screen('t("@[2J@[3;5HROLLED@[6;1H")',
               'clear and position after hardware scrolling', '    ROLLED', [0,0,264,40])
    elif name == 'history':
        s.enter('history=0')
        s.enter('history+=1; print("HISTORY",history)', 'HISTORY 1')
        s.key('Up'); s.enter('', 'HISTORY 2')
        s.ctrl('P'); s.ctrl('N'); s.check('Ctrl+N new input', suffix='>>>')
        s.ctrl('P'); s.enter('', 'HISTORY 3')
    elif name == 'completion':
        s.enter('completionvalue=37')
        s.text('print("COMPLETE",completi'); s.key('Tab'); s.enter(')', 'COMPLETE 37')
        s.enter('word="abc"')
        s.text('print("METHOD",word.upp'); s.key('Tab'); s.enter('())', 'METHOD ABC')
        s.text('if True:\r'); s.key('Tab'); s.enter('print("TAB",1)\r', 'TAB 1')
        s.enter('for i in range(900):\r t=list(range(12))\r'); s.wait(300)
        s.enter('assert completionvalue==37; print("ALIVE",1)', 'ALIVE 1')
        if variant == 'extra':
            s.enter('import micropython; events=[]; micropython.schedule(events.append,7); micropython.schedule(events.append,8)')
            s.enter('assert events==[7,8]; print("SCHEDULER",1)', 'SCHEDULER 1')
    elif name == 'interrupts':
        s.enter('completionvalue=37')
        s.text('unfinished draft'); s.ctrl('C')
        s.enter('print("CANCEL",1)', 'CANCEL 1')
        s.text('while True:\r pass\r\r'); s.wait(450); s.ctrl('C', hold=30)
        s.check('Ctrl+C interrupts execution', lines=['KeyboardInterrupt:'], suffix='>>>')
        s.enter('assert completionvalue==37; print("INTERRUPT",1)', 'INTERRUPT 1')
        if variant == 'extra':
            s.enter('import micropython; f=getattr(micropython,"kbd"+chr(95)+"intr"); print("HOOK",1)', 'HOOK 1')
            s.text('while True:\r f(24)\r\r'); s.wait(450); s.ctrl('X', hold=30)
            s.check('assigned Ctrl+X interrupt', lines=['KeyboardInterrupt:'], suffix='>>>')
            s.text('while True:\r f(120)\r\r'); s.wait(450); s.key('X', hold=30); s.wait()
            s.check('assigned plain x interrupt', lines=['KeyboardInterrupt:'], suffix='>>>')
    elif name == 'modes':
        s.enter('completionvalue=37'); s.ctrl('D')
        s.check('soft reboot banner', contains=['MicroPython'])
        s.enter('try:\r completionvalue\rexcept NameError:\r print("RESET",1)\r', 'RESET 1')
        s.ctrl('A'); s.check('raw REPL entry', contains=['raw REPL'])
        s.text('print("RAW",1)'); s.ctrl('D'); s.wait()
        s.check('raw REPL execution', lines=['>OKRAW 1'], suffix='>')
        s.ctrl('B'); s.check('friendly REPL return', contains=['MicroPython'], suffix='>>>')
        s.ctrl('E'); s.check('paste entry', contains=['paste mode'])
        s.text('print("PASTE",1)\r'); s.ctrl('D'); s.wait()
        s.check('paste execution', lines=['PASTE 1'], suffix='>>>')
        s.ctrl('E'); s.text('unfinished paste'); s.ctrl('C')
        s.enter('print("FRIENDLY",1)', 'FRIENDLY 1')
    elif name == 'logo-graphics':
        s.wait(150); s.graphics('initial north turtle', 'north')
        s.enter('ht'); s.graphics('hide turtle leaves empty canvas', 'blank')
        s.enter('repeat 4 [fd 20 rt 90]'); s.graphics('four complete square edges, empty interior', 'square20')
        s.enter('st'); s.graphics('show turtle over square', 'square20-north')
        s.enter('ht'); s.graphics('hide turtle restores covered square pixels', 'square20')
        for _ in range(5): s.enter('print 0')
        s.graphics('left transcript scrolling preserves graphics', 'square20')
        s.enter('cs'); s.graphics('clear square with turtle hidden', 'blank')
        s.enter('cs'); s.graphics('repeated clear stays empty', 'blank')
        s.enter('pu fd 10 pd rt 90 fd 15'); s.graphics('pen-up move and pen-down line', 'pen-line')
        s.enter('bk 5'); s.graphics('backward retraces existing line', 'pen-line')
        s.enter('home'); s.graphics('home draws diagonal to origin', 'home-line')
        s.enter('cs'); s.graphics('clear returns home', 'blank')
        s.enter('to q :n'); s.enter('repeat 4 [fd :n rt 90]'); s.enter('end')
        s.enter('q 10'); s.graphics('parameterized ten-step square', 'square10')
        s.enter('cs st'); s.graphics('clear and show north turtle', 'north')
        s.enter('rt 90'); s.graphics('right turn shows east turtle', 'east')
        s.enter('ht'); s.graphics('hide rotated turtle', 'blank')
        s.enter('st'); s.graphics('show rotated turtle', 'east')
        s.enter('lt 90'); s.graphics('left turn restores north turtle', 'north')
        s.enter('pu fd 10'); s.graphics('pen-up move leaves only relocated turtle', 'north-moved')
        s.enter('ct'); s.graphics('cleartext preserves graphics', 'north-moved')
    elif name == 'logo':
        # Fill the transcript so the live editor is always on row eight.
        # Decode only completed rows in the left pane; never decode turtle pixels.
        for _ in range(8): s.enter('print 0')
        s.enter('print 42', '42', 'Logo print')
        s.enter('repeat 4 [fd 20 rt 90]')
        s.check('square command transcript', lines=['? repeat 4 [fd 20 rt 9', '0]'], absent=["don't know", 'ERROR'])
        s.enter('to q :n'); s.check('procedure header transcript', contains=['to q :n'])
        s.check('definition prompt', lines=['>'], region=[0,56,6,8])
        s.enter('print :n'); s.enter('end'); s.enter('q 37', '37', 'parameterized procedure')
        s.enter('erase [q]'); s.enter('to q'); s.enter('print 43'); s.enter('end')
        s.enter('q', '43', 'redefined procedure')
        s.text('print 42x'); s.key('Backspace'); s.enter('', '42', 'Logo Backspace')
        s.text('discard this draft'); s.ctrl('U'); s.enter('print 44', '44', 'Logo Ctrl+U')
        s.enter('to unfinished'); s.text('print 99'); s.ctrl('C')
        s.check('cancel returns normal prompt', lines=['?'], region=[0,56,6,8])
        s.enter('print 45', '45', 'Logo cancel definition')
        for command in ['ct', 'cleartext']:
            s.enter(command)
            s.check(f'{command} clears transcript', region=[0,8,132,56], equals='')
            s.check(f'{command} restores top prompt', lines=['?'], region=[0,0,6,8])
            if command == 'ct': s.enter('print 46')
    else:
        raise ValueError(f'unknown scenario {name}')
    s.lua += ['finish()', 'run()']
    return s


def verify(out, checks):
    log = (out/'mame.log').read_text()
    if 'FAIL' in log or 'LUA ERROR' in log or 'PASS Neo scenario completed' not in log:
        raise ValueError('incomplete or failed guest scenario; see mame.log')
    results = []
    for index, spec in enumerate(checks, 1):
        paths = sorted((out/'snap').glob(f'checkpoint-{index:02d}-*.png'))
        if 'graphics' in spec:
            result = compare_graphics(paths, spec['graphics'], out/f'checkpoint-{index:02d}')
            results.append(dict(name=spec['name'], captures=[str(p) for p in paths], **result))
            continue
        reading, selected = stable_read(paths, region=spec['region'])
        text = reading.text
        (out/f'checkpoint-{index:02d}.txt').write_text(text+'\n')
        if spec.get('equals') is not None and text != spec['equals']:
            raise ValueError(f"{spec['name']}: expected exact text {spec['equals']!r}; got {text!r}")
        for line in spec['lines']:
            if line not in text.splitlines():
                raise ValueError(f"{spec['name']}: missing exact output line {line!r}; got {text!r}")
        if spec['suffix'] is not None and not text.endswith(spec['suffix']):
            raise ValueError(f"{spec['name']}: missing suffix {spec['suffix']!r}; got {text!r}")
        for fragment in spec['contains']:
            if fragment not in text:
                raise ValueError(f"{spec['name']}: missing {fragment!r}; got {text!r}")
        for fragment in spec['absent']:
            if fragment in text:
                raise ValueError(f"{spec['name']}: unexpected {fragment!r}")
        results.append(dict(name=spec['name'], selected=selected, text=text, region=spec['region'], font_sha256=reading.font_sha256))
    (out/'results.json').write_text(json.dumps(results, indent=2)+'\n')
    return len(results)


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--no-build', action='store_true', help='explicitly use existing applet binaries')
    p.add_argument('--case', choices=['all', 'editing', 'terminal', 'history', 'completion', 'interrupts', 'modes', 'logo', 'logo-graphics'], nargs='+', default=['all'])
    p.add_argument('--variant', choices=['minimal', 'core', 'basic', 'extra'], default='extra')
    p.add_argument('--mame', type=Path, default=ROOT/'mame/neodiag')
    p.add_argument('--firmware', type=Path, default=ROOT/'Ghidra/Alphasmart Neo/SmartApplets')
    p.add_argument('--output', type=Path)
    p.add_argument('--verify-only', type=Path)
    args = p.parse_args()
    if args.verify_only:
        try:
            count = verify(args.verify_only, json.loads((args.verify_only/'checks.json').read_text()))
            print(f'PASS {count} offline assertions: {args.verify_only}')
            return 0
        except (OSError, ValueError) as error:
            print(f'FAIL: {error}', file=sys.stderr)
            return 1
    out = args.output or Path(tempfile.mkdtemp(prefix='neo-repl-'))
    cases = ['editing', 'terminal', 'history', 'completion', 'interrupts', 'modes', 'logo', 'logo-graphics'] if 'all' in args.case else args.case
    try:
        out.mkdir(parents=True, exist_ok=True)
        if not args.no_build:
            build_images(args.variant, out/'build.log')
        for name in cases:
            session = (out/name).resolve()
            subprocess.run([sys.executable, str(HERE/'check.py'), '--prepare-only', '--no-build', '--variant', args.variant,
                            '--mame', str(args.mame), '--firmware', str(args.firmware), '--output', str(session)], check=True)
            s = scenario(name, args.variant)
            script = (HERE/'NeoHarness.lua').read_text()+'\n'+'\n'.join(s.lua)+'\n'
            (session/'scenario.lua').write_text(script)
            (session/'checks.json').write_text(json.dumps(s.checks, indent=2)+'\n')
            inputs = [HERE/'logo_graphics.py', *sorted(REFERENCES.glob('*.png')), HERE/'NeoHarness.lua', Path(__file__), HERE/'neo_reader.py', session/'checks.json', session/'scenario.lua']
            manifest = json.loads((session/'manifest.json').read_text())
            manifest.extend(dict(path=str(f), sha256=hashlib.sha256(f.read_bytes()).hexdigest()) for f in inputs)
            (session/'manifest.json').write_text(json.dumps(manifest, indent=2)+'\n')
            print(f'Running Neo {name} ({args.variant})...', flush=True)
            with (session/'mame.log').open('w') as log:
                result = subprocess.run([str(args.mame.resolve()), 'asneo', '-rompath', str(session/'roms'),
                    '-nvram_directory', str(session/'nvram'), '-cfg_directory', str(session/'cfg'),
                    '-snapshot_directory', str(session/'snap'), '-video', 'none', '-sound', 'none', '-nothrottle',
                    '-skip_gameinfo', '-autoboot_delay', '0', '-autoboot_script', str(session/'scenario.lua'),
                    '-seconds_to_run', '1500'], env=dict(os.environ, SDL_VIDEODRIVER='dummy'),
                    stdout=log, stderr=subprocess.STDOUT, timeout=900)
            if result.returncode:
                raise ValueError(f'MAME exited with {result.returncode}')
            count = verify(session, s.checks)
            print(f'PASS Neo {name}: {count} offline assertions; {session}', flush=True)
        return 0
    except (OSError, ValueError, AssertionError, subprocess.SubprocessError) as error:
        print(f'FAIL: {error}; artifacts: {out}', file=sys.stderr)
        return 1


if __name__ == '__main__':
    sys.exit(main())
