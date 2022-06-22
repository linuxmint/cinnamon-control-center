#!/usr/bin/python3

import os
import subprocess

# Symlinks desktop files to c-c-c's panel dir so the cinnamon-control-center binary can find the plugins.
destdir_prefix = os.environ.get('MESON_INSTALL_DESTDIR_PREFIX')
prefix = os.environ.get('MESON_INSTALL_PREFIX')

source_location = os.path.join(prefix, "share", "applications")
target_location = os.path.join(destdir_prefix, "share", "cinnamon-control-center", "panels")

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
