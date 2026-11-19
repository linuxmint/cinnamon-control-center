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
StartupNotify=true
Categories=GTK;Settings;X-Cinnamon-Settings-Panel;HardwareSettings
OnlyShowIn=X-Cinnamon;
X-Cinnamon-Settings-Panel=color
"""

additionalfiles.generate(DOMAIN, PATH, "./panels/color/cinnamon-color-panel.desktop", prefix, _("Color"), _("Color management settings"), "", keywords=_("Color, ICC, Profile, Calibrate, Printer, Display"))

prefix = """[Desktop Entry]
Exec=cinnamon-settings network
Icon=cs-network
Terminal=false
Type=Application
StartupNotify=true
Categories=GTK;Settings;HardwareSettings;X-Cinnamon-Settings-Panel;
OnlyShowIn=X-Cinnamon;
X-Cinnamon-Settings-Panel=network
"""

additionalfiles.generate(DOMAIN, PATH, "./panels/network/cinnamon-network-panel.desktop", prefix, _("Network"), _("Network settings"), "", keywords=_("Network, Wireless, IP, LAN, Proxy, Internet, WiFi"))

prefix = """[Desktop Entry]
Exec=cinnamon-settings wacom
Icon=cs-tablet
Terminal=false
Type=Application
StartupNotify=true
Categories=GNOME;GTK;Settings;HardwareSettings;X-Cinnamon-Settings-Panel;
X-Cinnamon-Settings-Panel=wacom
OnlyShowIn=X-Cinnamon;
"""

additionalfiles.generate(DOMAIN, PATH, "./panels/wacom/cinnamon-wacom-panel.desktop", prefix, _("Graphics Tablet"), _("Set button mappings and adjust stylus sensitivity for graphics tablets"), "", keywords=_("Tablet, Wacom, Stylus, Eraser, Mouse"))
