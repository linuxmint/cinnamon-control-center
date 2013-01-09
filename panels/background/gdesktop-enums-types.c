


#include <gdesktop-enums.h>
#include "gdesktop-enums-types.h"
#include "cc-background-item.h"

/* enumerations from "/home/hadess/Projects/gnome-install/include/gsettings-desktop-schemas/gdesktop-enums.h" */
GType
g_desktop_proxy_mode_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { G_DESKTOP_PROXY_MODE_NONE, "G_DESKTOP_PROXY_MODE_NONE", "none" },
      { G_DESKTOP_PROXY_MODE_MANUAL, "G_DESKTOP_PROXY_MODE_MANUAL", "manual" },
      { G_DESKTOP_PROXY_MODE_AUTO, "G_DESKTOP_PROXY_MODE_AUTO", "auto" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static ("GDesktopProxyMode", values);
  }
  return etype;
}
GType
g_desktop_toolbar_style_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { G_DESKTOP_TOOLBAR_STYLE_BOTH, "G_DESKTOP_TOOLBAR_STYLE_BOTH", "both" },
      { G_DESKTOP_TOOLBAR_STYLE_BOTH_HORIZ, "G_DESKTOP_TOOLBAR_STYLE_BOTH_HORIZ", "both-horiz" },
      { G_DESKTOP_TOOLBAR_STYLE_ICONS, "G_DESKTOP_TOOLBAR_STYLE_ICONS", "icons" },
      { G_DESKTOP_TOOLBAR_STYLE_TEXT, "G_DESKTOP_TOOLBAR_STYLE_TEXT", "text" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static ("GDesktopToolbarStyle", values);
  }
  return etype;
}
GType
g_desktop_toolbar_icon_size_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { G_DESKTOP_TOOLBAR_ICON_SIZE_SMALL, "G_DESKTOP_TOOLBAR_ICON_SIZE_SMALL", "small" },
      { G_DESKTOP_TOOLBAR_ICON_SIZE_LARGE, "G_DESKTOP_TOOLBAR_ICON_SIZE_LARGE", "large" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static ("GDesktopToolbarIconSize", values);
  }
  return etype;
}
GType
g_desktop_background_style_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { G_DESKTOP_BACKGROUND_STYLE_NONE, "G_DESKTOP_BACKGROUND_STYLE_NONE", "none" },
      { G_DESKTOP_BACKGROUND_STYLE_WALLPAPER, "G_DESKTOP_BACKGROUND_STYLE_WALLPAPER", "wallpaper" },
      { G_DESKTOP_BACKGROUND_STYLE_CENTERED, "G_DESKTOP_BACKGROUND_STYLE_CENTERED", "centered" },
      { G_DESKTOP_BACKGROUND_STYLE_SCALED, "G_DESKTOP_BACKGROUND_STYLE_SCALED", "scaled" },
      { G_DESKTOP_BACKGROUND_STYLE_STRETCHED, "G_DESKTOP_BACKGROUND_STYLE_STRETCHED", "stretched" },
      { G_DESKTOP_BACKGROUND_STYLE_ZOOM, "G_DESKTOP_BACKGROUND_STYLE_ZOOM", "zoom" },
      { G_DESKTOP_BACKGROUND_STYLE_SPANNED, "G_DESKTOP_BACKGROUND_STYLE_SPANNED", "spanned" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static ("GDesktopBackgroundStyle", values);
  }
  return etype;
}
GType
g_desktop_background_shading_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { G_DESKTOP_BACKGROUND_SHADING_SOLID, "G_DESKTOP_BACKGROUND_SHADING_SOLID", "solid" },
      { G_DESKTOP_BACKGROUND_SHADING_VERTICAL, "G_DESKTOP_BACKGROUND_SHADING_VERTICAL", "vertical" },
      { G_DESKTOP_BACKGROUND_SHADING_HORIZONTAL, "G_DESKTOP_BACKGROUND_SHADING_HORIZONTAL", "horizontal" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static ("GDesktopBackgroundShading", values);
  }
  return etype;
}
GType
g_desktop_mouse_dwell_mode_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { G_DESKTOP_MOUSE_DWELL_MODE_WINDOW, "G_DESKTOP_MOUSE_DWELL_MODE_WINDOW", "window" },
      { G_DESKTOP_MOUSE_DWELL_MODE_GESTURE, "G_DESKTOP_MOUSE_DWELL_MODE_GESTURE", "gesture" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static ("GDesktopMouseDwellMode", values);
  }
  return etype;
}
GType
g_desktop_mouse_dwell_direction_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { G_DESKTOP_MOUSE_DWELL_DIRECTION_LEFT, "G_DESKTOP_MOUSE_DWELL_DIRECTION_LEFT", "left" },
      { G_DESKTOP_MOUSE_DWELL_DIRECTION_RIGHT, "G_DESKTOP_MOUSE_DWELL_DIRECTION_RIGHT", "right" },
      { G_DESKTOP_MOUSE_DWELL_DIRECTION_UP, "G_DESKTOP_MOUSE_DWELL_DIRECTION_UP", "up" },
      { G_DESKTOP_MOUSE_DWELL_DIRECTION_DOWN, "G_DESKTOP_MOUSE_DWELL_DIRECTION_DOWN", "down" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static ("GDesktopMouseDwellDirection", values);
  }
  return etype;
}
GType
g_desktop_clock_format_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { G_DESKTOP_CLOCK_FORMAT_24H, "G_DESKTOP_CLOCK_FORMAT_24H", "24h" },
      { G_DESKTOP_CLOCK_FORMAT_12H, "G_DESKTOP_CLOCK_FORMAT_12H", "12h" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static ("GDesktopClockFormat", values);
  }
  return etype;
}
GType
g_desktop_screensaver_mode_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { G_DESKTOP_SCREENSAVER_MODE_BLANK_ONLY, "G_DESKTOP_SCREENSAVER_MODE_BLANK_ONLY", "blank-only" },
      { G_DESKTOP_SCREENSAVER_MODE_RANDOM, "G_DESKTOP_SCREENSAVER_MODE_RANDOM", "random" },
      { G_DESKTOP_SCREENSAVER_MODE_SINGLE, "G_DESKTOP_SCREENSAVER_MODE_SINGLE", "single" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static ("GDesktopScreensaverMode", values);
  }
  return etype;
}
GType
g_desktop_magnifier_mouse_tracking_mode_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { G_DESKTOP_MAGNIFIER_MOUSE_TRACKING_MODE_NONE, "G_DESKTOP_MAGNIFIER_MOUSE_TRACKING_MODE_NONE", "none" },
      { G_DESKTOP_MAGNIFIER_MOUSE_TRACKING_MODE_CENTERED, "G_DESKTOP_MAGNIFIER_MOUSE_TRACKING_MODE_CENTERED", "centered" },
      { G_DESKTOP_MAGNIFIER_MOUSE_TRACKING_MODE_PROPORTIONAL, "G_DESKTOP_MAGNIFIER_MOUSE_TRACKING_MODE_PROPORTIONAL", "proportional" },
      { G_DESKTOP_MAGNIFIER_MOUSE_TRACKING_MODE_PUSH, "G_DESKTOP_MAGNIFIER_MOUSE_TRACKING_MODE_PUSH", "push" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static ("GDesktopMagnifierMouseTrackingMode", values);
  }
  return etype;
}
GType
g_desktop_magnifier_screen_position_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { G_DESKTOP_MAGNIFIER_SCREEN_POSITION_NONE, "G_DESKTOP_MAGNIFIER_SCREEN_POSITION_NONE", "none" },
      { G_DESKTOP_MAGNIFIER_SCREEN_POSITION_FULL_SCREEN, "G_DESKTOP_MAGNIFIER_SCREEN_POSITION_FULL_SCREEN", "full-screen" },
      { G_DESKTOP_MAGNIFIER_SCREEN_POSITION_TOP_HALF, "G_DESKTOP_MAGNIFIER_SCREEN_POSITION_TOP_HALF", "top-half" },
      { G_DESKTOP_MAGNIFIER_SCREEN_POSITION_BOTTOM_HALF, "G_DESKTOP_MAGNIFIER_SCREEN_POSITION_BOTTOM_HALF", "bottom-half" },
      { G_DESKTOP_MAGNIFIER_SCREEN_POSITION_LEFT_HALF, "G_DESKTOP_MAGNIFIER_SCREEN_POSITION_LEFT_HALF", "left-half" },
      { G_DESKTOP_MAGNIFIER_SCREEN_POSITION_RIGHT_HALF, "G_DESKTOP_MAGNIFIER_SCREEN_POSITION_RIGHT_HALF", "right-half" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static ("GDesktopMagnifierScreenPosition", values);
  }
  return etype;
}
GType
g_desktop_titlebar_action_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { G_DESKTOP_TITLEBAR_ACTION_TOGGLE_SHADE, "G_DESKTOP_TITLEBAR_ACTION_TOGGLE_SHADE", "toggle-shade" },
      { G_DESKTOP_TITLEBAR_ACTION_TOGGLE_MAXIMIZE, "G_DESKTOP_TITLEBAR_ACTION_TOGGLE_MAXIMIZE", "toggle-maximize" },
      { G_DESKTOP_TITLEBAR_ACTION_TOGGLE_MAXIMIZE_HORIZONTALLY, "G_DESKTOP_TITLEBAR_ACTION_TOGGLE_MAXIMIZE_HORIZONTALLY", "toggle-maximize-horizontally" },
      { G_DESKTOP_TITLEBAR_ACTION_TOGGLE_MAXIMIZE_VERTICALLY, "G_DESKTOP_TITLEBAR_ACTION_TOGGLE_MAXIMIZE_VERTICALLY", "toggle-maximize-vertically" },
      { G_DESKTOP_TITLEBAR_ACTION_MINIMIZE, "G_DESKTOP_TITLEBAR_ACTION_MINIMIZE", "minimize" },
      { G_DESKTOP_TITLEBAR_ACTION_NONE, "G_DESKTOP_TITLEBAR_ACTION_NONE", "none" },
      { G_DESKTOP_TITLEBAR_ACTION_LOWER, "G_DESKTOP_TITLEBAR_ACTION_LOWER", "lower" },
      { G_DESKTOP_TITLEBAR_ACTION_MENU, "G_DESKTOP_TITLEBAR_ACTION_MENU", "menu" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static ("GDesktopTitlebarAction", values);
  }
  return etype;
}
GType
g_desktop_focus_mode_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { G_DESKTOP_FOCUS_MODE_CLICK, "G_DESKTOP_FOCUS_MODE_CLICK", "click" },
      { G_DESKTOP_FOCUS_MODE_SLOPPY, "G_DESKTOP_FOCUS_MODE_SLOPPY", "sloppy" },
      { G_DESKTOP_FOCUS_MODE_MOUSE, "G_DESKTOP_FOCUS_MODE_MOUSE", "mouse" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static ("GDesktopFocusMode", values);
  }
  return etype;
}
GType
g_desktop_focus_new_windows_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { G_DESKTOP_FOCUS_NEW_WINDOWS_SMART, "G_DESKTOP_FOCUS_NEW_WINDOWS_SMART", "smart" },
      { G_DESKTOP_FOCUS_NEW_WINDOWS_STRICT, "G_DESKTOP_FOCUS_NEW_WINDOWS_STRICT", "strict" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static ("GDesktopFocusNewWindows", values);
  }
  return etype;
}
GType
g_desktop_visual_bell_type_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { G_DESKTOP_VISUAL_BELL_FULLSCREEN_FLASH, "G_DESKTOP_VISUAL_BELL_FULLSCREEN_FLASH", "fullscreen-flash" },
      { G_DESKTOP_VISUAL_BELL_FRAME_FLASH, "G_DESKTOP_VISUAL_BELL_FRAME_FLASH", "frame-flash" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static ("GDesktopVisualBellType", values);
  }
  return etype;
}

/* enumerations from "cc-background-item.h" */
GType
cc_background_item_flags_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GFlagsValue values[] = {
      { CC_BACKGROUND_ITEM_HAS_SHADING, "CC_BACKGROUND_ITEM_HAS_SHADING", "shading" },
      { CC_BACKGROUND_ITEM_HAS_PLACEMENT, "CC_BACKGROUND_ITEM_HAS_PLACEMENT", "placement" },
      { CC_BACKGROUND_ITEM_HAS_PCOLOR, "CC_BACKGROUND_ITEM_HAS_PCOLOR", "pcolor" },
      { CC_BACKGROUND_ITEM_HAS_SCOLOR, "CC_BACKGROUND_ITEM_HAS_SCOLOR", "scolor" },
      { CC_BACKGROUND_ITEM_HAS_URI, "CC_BACKGROUND_ITEM_HAS_URI", "uri" },
      { 0, NULL, NULL }
    };
    etype = g_flags_register_static ("CcBackgroundItemFlags", values);
  }
  return etype;
}



