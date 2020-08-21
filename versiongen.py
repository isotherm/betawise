#!/usr/bin/env python

import subprocess

slug = subprocess.check_output(['git', 'describe', '--tags'])
if '-' in slug:
    tag, commits, hash = slug.split('-')
    version_revision = chr(0x60 + int(commits))
else:
    tag = slug
    version_revision = ' '

version_major, version_minor = map(int, tag[1:].split('.'))
if tag[1:].split('.')[0] == '0':
    version_minor = 0

print('#define BETAWISE_VERSION_MAJOR %d' % version_major)
print('#define BETAWISE_VERSION_MINOR %d' % version_minor)
print('#define BETAWISE_VERSION_REVISION "%c"' % version_revision)
