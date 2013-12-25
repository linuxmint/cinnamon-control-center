/* We copy csd-wacom-device from gnome-settings-daemon.
 * It include "csd-enums.h" because the include directory
 * is known. As gnome-settings-daemon's pkg-config file
 * prefixes this, we need a little help to avoid this
 * one line difference */

#include <cinnamon-settings-daemon/csd-enums.h>
