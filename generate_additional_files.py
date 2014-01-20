#!/usr/bin/python

import os, gettext

DOMAIN = "cinnamon-control-center"
PATH = "/usr/share/cinnamon/locale"

def generate(filename, prefix, name, comment, suffix):
    gettext.install(DOMAIN, PATH)
    desktopFile = open(filename, "w")

    desktopFile.writelines(prefix)

    desktopFile.writelines("Name=%s\n" % name)
    for directory in sorted(os.listdir(PATH)):
        if os.path.isdir(os.path.join(PATH, directory)):
            try:
                language = gettext.translation(DOMAIN, PATH, languages=[directory])
                language.install()
                if (_(name) != name):
                    desktopFile.writelines("Name[%s]=%s\n" % (directory, _(name)))
            except:
                pass

    desktopFile.writelines("Comment=%s\n" % comment)
    for directory in sorted(os.listdir(PATH)):
        if os.path.isdir(os.path.join(PATH, directory)):
            try:
                language = gettext.translation(DOMAIN, PATH, languages=[directory])
                language.install()
                if (_(comment) != comment):
                    desktopFile.writelines("Comment[%s]=%s\n" % (directory, _(comment)))
            except:
                pass

    desktopFile.writelines(suffix)

os.environ['LANG'] = "en"
gettext.install(DOMAIN, PATH)

prefix = """[Desktop Entry]
Exec=cinnamon-control-center screen
Icon=system-lock-screen
Terminal=false
Type=Application
StartupNotify=true
Categories=GTK;Settings;DesktopSettings;X-Cinnamon-Settings-Panel;X-Cinnamon-PersonalSettings
OnlyShowIn=X-Cinnamon;
X-Cinnamon-Settings-Panel=screen
# Translators: those are keywords for the brightness and lock control-center panel
_Keywords=Brightness;Lock;Dim;Blank;Monitor;
NoDisplay=true
"""

generate("./panels/screen/cinnamon-screen-panel.desktop.in.in", prefix, _("Brightness & Lock"), _("Screen brightness and lock settings"), "")

prefix = """[Desktop Entry]
Exec=cinnamon-settings sound
Icon=multimedia-volume-control
Terminal=false
Type=Application
StartupNotify=true
Categories=GTK;Settings;HardwareSettings;X-Cinnamon-Settings-Panel;
OnlyShowIn=X-Cinnamon;
X-Cinnamon-Settings-Panel=sound-nua
# Translators: those are keywords for the sound control-center panel
_Keywords=Card;Microphone;Volume;Fade;Balance;Bluetooth;Headset;
"""

generate("./panels/sound-nua/data/cinnamon-sound-nua-panel.desktop.in.in", prefix, _("Sound"), _("Change sound volume and sound events"), "")

prefix = """[Desktop Entry]
Exec=cinnamon-settings color
Icon=cinnamon-preferences-color
Terminal=false
Type=Application
StartupNotify=true
Categories=GTK;Settings;X-Cinnamon-Settings-Panel;HardwareSettings
OnlyShowIn=X-Cinnamon;
X-Cinnamon-Settings-Panel=color
# Translators: those are keywords for the color control-center panel
_Keywords=Color;ICC;Profile;Calibrate;Printer;Display;
"""

generate("./panels/color/cinnamon-color-panel.desktop.in.in", prefix, _("Color"), _("Color management settings"), "")

prefix = """[Desktop Entry]
Exec=cinnamon-control-center sound
Icon=multimedia-volume-control
Terminal=false
Type=Application
StartupNotify=true
Categories=GTK;Settings;HardwareSettings;X-Cinnamon-Settings-Panel;
OnlyShowIn=X-Cinnamon;
NoDisplay=true
X-Cinnamon-Settings-Panel=sound
# Translators: those are keywords for the sound control-center panel
_Keywords=Card;Microphone;Volume;Fade;Balance;Bluetooth;Headset;Audio;
"""

generate("./panels/sound/data/cinnamon-sound-panel.desktop.in.in", prefix, _("Sound"), _("Change sound volume and sound events"), "")

prefix = """[Desktop Entry]
Icon=multimedia-volume-control
Exec=cinnamon-sound-applet
Terminal=false
Type=Application
Categories=
NoDisplay=true
X-GNOME-Autostart-Notify=true
AutostartCondition=GNOME3 if-session gnome-fallback
OnlyShowIn=X-Cinnamon;
"""

generate("./panels/sound/data/cinnamon-sound-applet.desktop.in", prefix, _("Volume Control"), _("Show desktop volume control"), "")

prefix = """[Desktop Entry]
Exec=cinnamon-settings power
Icon=cinnamon-power-manager
Terminal=false
Type=Application
StartupNotify=true
Categories=GTK;Settings;DesktopSettings;X-Cinnamon-Settings-Panel;HardwareSettings
OnlyShowIn=X-Cinnamon;
X-Cinnamon-Settings-Panel=power
# Translators: those are keywords for the power control-center panel
_Keywords=Power;Sleep;Suspend;Hibernate;Battery;
"""

generate("./panels/power/cinnamon-power-panel.desktop.in.in", prefix, _("Power"), _("Power management settings"), "")

prefix = """[Desktop Entry]
Exec=cinnamon-settings region
Icon=preferences-desktop-locale
Terminal=false
Type=Application
StartupNotify=true
Categories=GTK;Settings;DesktopSettings;X-Cinnamon-Settings-Panel;X-Cinnamon-PersonalSettings
OnlyShowIn=X-Cinnamon;
X-Cinnamon-Settings-Panel=region
# Translators: those are keywords for the region control-center panel
_Keywords=Language;Layout;Keyboard;
"""

generate("./panels/region/cinnamon-region-panel.desktop.in.in", prefix, _("Region & Language"), _("Change your region and language settings"), "")

prefix = """[Desktop Entry]
Exec=cinnamon-settings display
Icon=preferences-desktop-display
Terminal=false
Type=Application
StartupNotify=true
Categories=GTK;Settings;HardwareSettings;X-Cinnamon-Settings-Panel;
OnlyShowIn=X-Cinnamon;
X-Cinnamon-Settings-Panel=display
# Translators: those are keywords for the display control-center panel
_Keywords=Panel;Projector;xrandr;Screen;Resolution;Refresh;
"""

generate("./panels/display/cinnamon-display-panel.desktop.in.in", prefix, _("Displays"), _("Change resolution and position of monitors and projectors"), "")

prefix = """[Desktop Entry]
Exec=cinnamon-settings network
Icon=network-workgroup
Terminal=false
Type=Application
StartupNotify=true
Categories=GTK;Settings;HardwareSettings;X-Cinnamon-Settings-Panel;
OnlyShowIn=X-Cinnamon;
X-Cinnamon-Settings-Panel=network
# Translators: those are keywords for the network control-center panel
_Keywords=Network;Wireless;IP;LAN;Proxy;
"""

generate("./panels/network/cinnamon-network-panel.desktop.in.in", prefix, _("Network"), _("Network settings"), "")

prefix = """[Desktop Entry]
Exec=cinnamon-settings universal-access
Icon=preferences-desktop-accessibility
Terminal=false
Type=Application
StartupNotify=true
Categories=GTK;Settings;X-Cinnamon-SystemSettings;X-Cinnamon-Settings-Panel;
OnlyShowIn=X-Cinnamon;
X-Cinnamon-Settings-Panel=universal-access
# Translators: those are keywords for the universal access control-center panel
_Keywords=Keyboard;Mouse;a11y;Accessibility;Contrast;Zoom;Screen Reader;text;font;size;AccessX;Sticky Keys;Slow Keys;Bounce Keys;Mouse Keys;
"""

generate("./panels/universal-access/cinnamon-universal-access-panel.desktop.in.in", prefix, _("Universal Access"), _("Universal Access Preferences"), "")
