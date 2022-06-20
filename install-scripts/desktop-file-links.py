#!/usr/bin/python3

import os
import subprocess

# Symlinks desktop files to c-c-c's panel dir so the cinnamon-control-center binary can find the plugins.
dest = os.environ.get('DESTDIR')
prefix = os.environ.get('MESON_INSTALL_PREFIX')

if dest:
    root = dest
else:
    root = "/"

source_location = os.path.join(root, prefix[1:], "share", "applications")
target_location = os.path.join(root, prefix[1:], "share", "cinnamon-control-center", "panels")

links = [
    "cinnamon-color-panel.desktop",
    "cinnamon-display-panel.desktop",
    "cinnamon-network-panel.desktop",
    "cinnamon-wacom-panel.desktop"
]

for link in links:
    orig_path = os.path.join(source_location, link)
    link_path = os.path.join(target_location, link)

    if os.path.lexists(link_path):
        print('%s already exists, skipping symlink creation' % link_path)
        continue

    subprocess.call(['ln', '-s', orig_path, link_path])

exit(0)