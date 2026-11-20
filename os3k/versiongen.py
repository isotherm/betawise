#!/usr/bin/env python3

import subprocess

slug = subprocess.getoutput('git describe --tags')
if '-' in slug:
    tag, commits, hash = slug.split('-')
    commit_count = int(commits)
    # One header byte: 'a'..'z' for 1..26 commits past the tag, '+' beyond that.
    version_revision = chr(0x60 + commit_count) if 1 <= commit_count <= 26 else '+'
else:
    tag = slug
    version_revision = ' '
version_major, version_minor = map(int, tag[1:].split('.'))

print('#define BETAWISE_VERSION_MAJOR %d' % version_major)
print('#define BETAWISE_VERSION_MINOR %d' % version_minor)
print('#define BETAWISE_VERSION_REVISION "%c"' % version_revision)
