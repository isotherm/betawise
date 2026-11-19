#!/usr/bin/env python3
"""Check Doxygen syntax and API parameter/return coverage; retain no generated docs."""
from pathlib import Path
import re
import subprocess
import tempfile
import xml.etree.ElementTree as ET

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[2]
# Limit this audit to the syscall API, rather than unrelated experimental drivers.
headers = [p for p in (REPO / 'os3k').glob('os3k*.h')
           if 'internal' not in p.name and p.name != 'os3k_flash.h']
with tempfile.TemporaryDirectory(prefix='os3k-docs-') as tmp:
    output = Path(tmp)
    config = output / 'Doxyfile'
    config.write_text('\n'.join([
        'PROJECT_NAME = Betawise', f'OUTPUT_DIRECTORY = {output}',
        'INPUT = ' + ' '.join(str(p) for p in sorted(headers)),
        'QUIET = YES', 'GENERATE_HTML = YES', 'GENERATE_XML = YES',
        'GENERATE_LATEX = NO', 'EXTRACT_ALL = NO',
        'WARN_IF_DOC_ERROR = YES', 'WARN_NO_PARAMDOC = YES',
        'WARN_AS_ERROR = FAIL_ON_WARNINGS',
    ]) + '\n')
    # Doxygen 1.9.4 reports spurious missing-parameter warnings with HTML disabled.
    subprocess.run(['doxygen', str(config)], check=True)
    members = {}
    for p in (output / 'xml').glob('os3k*.xml'):
        for member in ET.parse(p).findall('.//memberdef[@kind="function"]'):
            members[member.findtext('name')] = member
    symbols = re.findall(r'^DEFINE_SYSCALL(?:_NEO)?\(\d+,\s*(\w+)\)',
                         (REPO / 'os3k/syscall.c').read_text(), re.M)
    failures = []
    for raw in symbols:
        public = raw if raw in members else raw.removeprefix('_OS3K_')
        if public not in members:
            failures.append(f'{raw}: no public declaration')
            continue
        member = members[public]
        brief = member.find('briefdescription')
        if brief is None or not ''.join(brief.itertext()).strip():
            failures.append(f'{public}: no Doxygen brief')
        params = {p.findtext('declname') for p in member.findall('param')
                  if p.findtext('type') != 'void'} - {None}
        docs = {p.text for p in member.findall('.//parameterlist[@kind="param"]//parametername')}
        if params - docs:
            failures.append(f'{public}: undocumented parameters {sorted(params - docs)}')
        if member.findtext('type') != 'void' and not (
                member.find('.//simplesect[@kind="return"]') is not None
                or member.find('.//parameterlist[@kind="retval"]') is not None):
            failures.append(f'{public}: undocumented return')
    assert not failures, '\n'.join(failures)
    print(f'PASS Doxygen syntax and public documentation for {len(symbols)} syscalls')
