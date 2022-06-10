#!/usr/bin/python3

DOMAIN = "cinnamon-control-center"
PATH = "/usr/share/locale"

import os, gettext, sys
from mintcommon import additionalfiles

os.environ['LANGUAGE'] = "en_US.UTF-8"
gettext.install(DOMAIN, PATH)

prefix = """[Desktop Entry]
Exec=cinnamon-settings color
Icon=cs-color
Terminal=false
Type=Application
X-Cinnamon-Settings-Panel=color
"""

additionalfiles.generate(DOMAIN, PATH, "./panels/color/cinnamon-color-panel.desktop", prefix, _("Color"), _("Color management settings"), "")

prefix = """[Desktop Entry]
Exec=cinnamon-settings region
Icon=cs-language
Terminal=false
Type=Application
X-Cinnamon-Settings-Panel=region
"""

additionalfiles.generate(DOMAIN, PATH, "./panels/region/cinnamon-region-panel.desktop", prefix, _("Region & Language"), _("Change your region and language settings"), "")

prefix = """[Desktop Entry]
Exec=cinnamon-control-center display
Icon=cs-display
Terminal=false
Type=Application
X-Cinnamon-Settings-Panel=display
"""

additionalfiles.generate(DOMAIN, PATH, "./panels/display/cinnamon-display-panel.desktop", prefix, _("Display"), _("Change resolution and position of monitors and projectors"), "")

prefix = """[Desktop Entry]
Exec=cinnamon-control-center network
Icon=cs-network
Terminal=false
Type=Application
X-Cinnamon-Settings-Panel=network
"""

additionalfiles.generate(DOMAIN, PATH, "./panels/network/cinnamon-network-panel.desktop", prefix, _("Network"), _("Network settings"), "")

prefix = """[Desktop Entry]
Exec=cinnamon-settings wacom
Icon=cs-tablet
Terminal=false
Type=Application
X-Cinnamon-Settings-Panel=wacom
"""

additionalfiles.generate(DOMAIN, PATH, "./panels/wacom/cinnamon-wacom-panel.desktop", prefix, _("Graphics Tablet"), _("Set button mappings and adjust stylus sensitivity for graphics tablets"), "")
