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
# Translators: those are keywords for the color control-center panel
_Keywords=Color;ICC;Profile;Calibrate;Printer;Display;
"""

additionalfiles.generate(DOMAIN, PATH, "./panels/color/cinnamon-color-panel.desktop.in.in", prefix, _("Color"), _("Color management settings"), "")

prefix = """[Desktop Entry]
Exec=cinnamon-settings region
Icon=cs-language
Terminal=false
Type=Application
StartupNotify=true
Categories=GTK;Settings;DesktopSettings;X-Cinnamon-Settings-Panel;X-Cinnamon-PersonalSettings
OnlyShowIn=X-Cinnamon;
X-Cinnamon-Settings-Panel=region
# Translators: those are keywords for the region control-center panel
_Keywords=Language;Layout;Keyboard;
NoDisplay=true
"""

additionalfiles.generate(DOMAIN, PATH, "./panels/region/cinnamon-region-panel.desktop.in.in", prefix, _("Region & Language"), _("Change your region and language settings"), "")

prefix = """[Desktop Entry]
Exec=cinnamon-settings display
Icon=cs-display
Terminal=false
Type=Application
StartupNotify=true
Categories=GTK;Settings;HardwareSettings;X-Cinnamon-Settings-Panel;
OnlyShowIn=X-Cinnamon;
X-Cinnamon-Settings-Panel=display
# Translators: those are keywords for the display control-center panel
_Keywords=Panel;Projector;xrandr;Screen;Resolution;Refresh;
"""

additionalfiles.generate(DOMAIN, PATH, "./panels/display/cinnamon-display-panel.desktop.in.in", prefix, _("Display"), _("Change resolution and position of monitors and projectors"), "")

prefix = """[Desktop Entry]
Exec=cinnamon-settings network
Icon=cs-network
Terminal=false
Type=Application
StartupNotify=true
Categories=GTK;Settings;HardwareSettings;X-Cinnamon-Settings-Panel;
OnlyShowIn=X-Cinnamon;
X-Cinnamon-Settings-Panel=network
# Translators: those are keywords for the network control-center panel
_Keywords=Network;Wireless;IP;LAN;Proxy;Internet;WiFi;
"""

additionalfiles.generate(DOMAIN, PATH, "./panels/network/cinnamon-network-panel.desktop.in.in", prefix, _("Network"), _("Network settings"), "")

prefix = """[Desktop Entry]
Exec=cinnamon-settings wacom
Icon=cs-tablet
Terminal=false
Type=Application
StartupNotify=true
Categories=GNOME;GTK;Settings;HardwareSettings;X-Cinnamon-Settings-Panel;
X-Cinnamon-Settings-Panel=wacom
OnlyShowIn=X-Cinnamon;
# Translators: those are keywords for the wacom tablet control-center panel
_Keywords=Tablet;Wacom;Stylus;Eraser;Mouse;
"""

additionalfiles.generate(DOMAIN, PATH, "./panels/wacom/cinnamon-wacom-panel.desktop.in.in", prefix, _("Graphics Tablet"), _("Set button mappings and adjust stylus sensitivity for graphics tablets"), "")
