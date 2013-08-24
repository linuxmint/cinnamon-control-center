


#include <libcinnamon-desktop/cdesktop-enums.hs>
#include "gdesktop-enums-types.h"
#include "cc-background-item.h"

/* enumerations from "/usr/include/gsettings-desktop-schemas/libcinnamon-desktop/cdesktop-enums.hs" */
GType
g_desktop_proxy_mode_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { C_DESKTOP_PROXY_MODE_NONE, "C_DESKTOP_PROXY_MODE_NONE", "none" },
      { C_DESKTOP_PROXY_MODE_MANUAL, "C_DESKTOP_PROXY_MODE_MANUAL", "manual" },
      { C_DESKTOP_PROXY_MODE_AUTO, "C_DESKTOP_PROXY_MODE_AUTO", "auto" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static ("CDesktopProxyMode", values);
  }
  return etype;
}
GType
g_desktop_toolbar_style_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { C_DESKTOP_TOOLBAR_STYLE_BOTH, "C_DESKTOP_TOOLBAR_STYLE_BOTH", "both" },
      { C_DESKTOP_TOOLBAR_STYLE_BOTH_HORIZ, "C_DESKTOP_TOOLBAR_STYLE_BOTH_HORIZ", "both-horiz" },
      { C_DESKTOP_TOOLBAR_STYLE_ICONS, "C_DESKTOP_TOOLBAR_STYLE_ICONS", "icons" },
      { C_DESKTOP_TOOLBAR_STYLE_TEXT, "C_DESKTOP_TOOLBAR_STYLE_TEXT", "text" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static ("CDesktopToolbarStyle", values);
  }
  return etype;
}
GType
g_desktop_toolbar_icon_size_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { C_DESKTOP_TOOLBAR_ICON_SIZE_SMALL, "C_DESKTOP_TOOLBAR_ICON_SIZE_SMALL", "small" },
      { C_DESKTOP_TOOLBAR_ICON_SIZE_LARGE, "C_DESKTOP_TOOLBAR_ICON_SIZE_LARGE", "large" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static ("CDesktopToolbarIconSize", values);
  }
  return etype;
}
GType
g_desktop_background_style_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { C_DESKTOP_BACKGROUND_STYLE_NONE, "C_DESKTOP_BACKGROUND_STYLE_NONE", "none" },
      { C_DESKTOP_BACKGROUND_STYLE_WALLPAPER, "C_DESKTOP_BACKGROUND_STYLE_WALLPAPER", "wallpaper" },
      { C_DESKTOP_BACKGROUND_STYLE_CENTERED, "C_DESKTOP_BACKGROUND_STYLE_CENTERED", "centered" },
      { C_DESKTOP_BACKGROUND_STYLE_SCALED, "C_DESKTOP_BACKGROUND_STYLE_SCALED", "scaled" },
      { C_DESKTOP_BACKGROUND_STYLE_STRETCHED, "C_DESKTOP_BACKGROUND_STYLE_STRETCHED", "stretched" },
      { C_DESKTOP_BACKGROUND_STYLE_ZOOM, "C_DESKTOP_BACKGROUND_STYLE_ZOOM", "zoom" },
      { C_DESKTOP_BACKGROUND_STYLE_SPANNED, "C_DESKTOP_BACKGROUND_STYLE_SPANNED", "spanned" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static ("CDesktopBackgroundStyle", values);
  }
  return etype;
}
GType
g_desktop_background_shading_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { C_DESKTOP_BACKGROUND_SHADING_SOLID, "C_DESKTOP_BACKGROUND_SHADING_SOLID", "solid" },
      { C_DESKTOP_BACKGROUND_SHADING_VERTICAL, "C_DESKTOP_BACKGROUND_SHADING_VERTICAL", "vertical" },
      { C_DESKTOP_BACKGROUND_SHADING_HORIZONTAL, "C_DESKTOP_BACKGROUND_SHADING_HORIZONTAL", "horizontal" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static ("CDesktopBackgroundShading", values);
  }
  return etype;
}
GType
g_desktop_mouse_dwell_mode_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { C_DESKTOP_MOUSE_DWELL_MODE_WINDOW, "C_DESKTOP_MOUSE_DWELL_MODE_WINDOW", "window" },
      { C_DESKTOP_MOUSE_DWELL_MODE_GESTURE, "C_DESKTOP_MOUSE_DWELL_MODE_GESTURE", "gesture" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static ("CDesktopMouseDwellMode", values);
  }
  return etype;
}
GType
g_desktop_mouse_dwell_direction_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { C_DESKTOP_MOUSE_DWELL_DIRECTION_LEFT, "C_DESKTOP_MOUSE_DWELL_DIRECTION_LEFT", "left" },
      { C_DESKTOP_MOUSE_DWELL_DIRECTION_RIGHT, "C_DESKTOP_MOUSE_DWELL_DIRECTION_RIGHT", "right" },
      { C_DESKTOP_MOUSE_DWELL_DIRECTION_UP, "C_DESKTOP_MOUSE_DWELL_DIRECTION_UP", "up" },
      { C_DESKTOP_MOUSE_DWELL_DIRECTION_DOWN, "C_DESKTOP_MOUSE_DWELL_DIRECTION_DOWN", "down" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static ("CDesktopMouseDwellDirection", values);
  }
  return etype;
}
GType
g_desktop_clock_format_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { C_DESKTOP_CLOCK_FORMAT_24H, "C_DESKTOP_CLOCK_FORMAT_24H", "24h" },
      { C_DESKTOP_CLOCK_FORMAT_12H, "C_DESKTOP_CLOCK_FORMAT_12H", "12h" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static ("CDesktopClockFormat", values);
  }
  return etype;
}
GType
g_desktop_screensaver_mode_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { C_DESKTOP_SCREENSAVER_MODE_BLANK_ONLY, "C_DESKTOP_SCREENSAVER_MODE_BLANK_ONLY", "blank-only" },
      { C_DESKTOP_SCREENSAVER_MODE_RANDOM, "C_DESKTOP_SCREENSAVER_MODE_RANDOM", "random" },
      { C_DESKTOP_SCREENSAVER_MODE_SINGLE, "C_DESKTOP_SCREENSAVER_MODE_SINGLE", "single" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static ("CDesktopScreensaverMode", values);
  }
  return etype;
}
GType
g_desktop_magnifier_mouse_tracking_mode_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { C_DESKTOP_MAGNIFIER_MOUSE_TRACKING_MODE_NONE, "C_DESKTOP_MAGNIFIER_MOUSE_TRACKING_MODE_NONE", "none" },
      { C_DESKTOP_MAGNIFIER_MOUSE_TRACKING_MODE_CENTERED, "C_DESKTOP_MAGNIFIER_MOUSE_TRACKING_MODE_CENTERED", "centered" },
      { C_DESKTOP_MAGNIFIER_MOUSE_TRACKING_MODE_PROPORTIONAL, "C_DESKTOP_MAGNIFIER_MOUSE_TRACKING_MODE_PROPORTIONAL", "proportional" },
      { C_DESKTOP_MAGNIFIER_MOUSE_TRACKING_MODE_PUSH, "C_DESKTOP_MAGNIFIER_MOUSE_TRACKING_MODE_PUSH", "push" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static ("CDesktopMagnifierMouseTrackingMode", values);
  }
  return etype;
}
GType
g_desktop_magnifier_screen_position_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { C_DESKTOP_MAGNIFIER_SCREEN_POSITION_NONE, "C_DESKTOP_MAGNIFIER_SCREEN_POSITION_NONE", "none" },
      { C_DESKTOP_MAGNIFIER_SCREEN_POSITION_FULL_SCREEN, "C_DESKTOP_MAGNIFIER_SCREEN_POSITION_FULL_SCREEN", "full-screen" },
      { C_DESKTOP_MAGNIFIER_SCREEN_POSITION_TOP_HALF, "C_DESKTOP_MAGNIFIER_SCREEN_POSITION_TOP_HALF", "top-half" },
      { C_DESKTOP_MAGNIFIER_SCREEN_POSITION_BOTTOM_HALF, "C_DESKTOP_MAGNIFIER_SCREEN_POSITION_BOTTOM_HALF", "bottom-half" },
      { C_DESKTOP_MAGNIFIER_SCREEN_POSITION_LEFT_HALF, "C_DESKTOP_MAGNIFIER_SCREEN_POSITION_LEFT_HALF", "left-half" },
      { C_DESKTOP_MAGNIFIER_SCREEN_POSITION_RIGHT_HALF, "C_DESKTOP_MAGNIFIER_SCREEN_POSITION_RIGHT_HALF", "right-half" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static ("CDesktopMagnifierScreenPosition", values);
  }
  return etype;
}
GType
g_desktop_titlebar_action_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { C_DESKTOP_TITLEBAR_ACTION_TOGGLE_SHADE, "C_DESKTOP_TITLEBAR_ACTION_TOGGLE_SHADE", "toggle-shade" },
      { C_DESKTOP_TITLEBAR_ACTION_TOGGLE_MAXIMIZE, "C_DESKTOP_TITLEBAR_ACTION_TOGGLE_MAXIMIZE", "toggle-maximize" },
      { C_DESKTOP_TITLEBAR_ACTION_TOGGLE_MAXIMIZE_HORIZONTALLY, "C_DESKTOP_TITLEBAR_ACTION_TOGGLE_MAXIMIZE_HORIZONTALLY", "toggle-maximize-horizontally" },
      { C_DESKTOP_TITLEBAR_ACTION_TOGGLE_MAXIMIZE_VERTICALLY, "C_DESKTOP_TITLEBAR_ACTION_TOGGLE_MAXIMIZE_VERTICALLY", "toggle-maximize-vertically" },
      { C_DESKTOP_TITLEBAR_ACTION_MINIMIZE, "C_DESKTOP_TITLEBAR_ACTION_MINIMIZE", "minimize" },
      { C_DESKTOP_TITLEBAR_ACTION_NONE, "C_DESKTOP_TITLEBAR_ACTION_NONE", "none" },
      { C_DESKTOP_TITLEBAR_ACTION_LOWER, "C_DESKTOP_TITLEBAR_ACTION_LOWER", "lower" },
      { C_DESKTOP_TITLEBAR_ACTION_MENU, "C_DESKTOP_TITLEBAR_ACTION_MENU", "menu" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static ("CDesktopTitlebarAction", values);
  }
  return etype;
}
GType
g_desktop_focus_mode_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { C_DESKTOP_FOCUS_MODE_CLICK, "C_DESKTOP_FOCUS_MODE_CLICK", "click" },
      { C_DESKTOP_FOCUS_MODE_SLOPPY, "C_DESKTOP_FOCUS_MODE_SLOPPY", "sloppy" },
      { C_DESKTOP_FOCUS_MODE_MOUSE, "C_DESKTOP_FOCUS_MODE_MOUSE", "mouse" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static ("CDesktopFocusMode", values);
  }
  return etype;
}
GType
g_desktop_focus_new_windows_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { C_DESKTOP_FOCUS_NEW_WINDOWS_SMART, "C_DESKTOP_FOCUS_NEW_WINDOWS_SMART", "smart" },
      { C_DESKTOP_FOCUS_NEW_WINDOWS_STRICT, "C_DESKTOP_FOCUS_NEW_WINDOWS_STRICT", "strict" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static ("CDesktopFocusNewWindows", values);
  }
  return etype;
}
GType
g_desktop_visual_bell_type_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { C_DESKTOP_VISUAL_BELL_FULLSCREEN_FLASH, "C_DESKTOP_VISUAL_BELL_FULLSCREEN_FLASH", "fullscreen-flash" },
      { C_DESKTOP_VISUAL_BELL_FRAME_FLASH, "C_DESKTOP_VISUAL_BELL_FRAME_FLASH", "frame-flash" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static ("CDesktopVisualBellType", values);
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



