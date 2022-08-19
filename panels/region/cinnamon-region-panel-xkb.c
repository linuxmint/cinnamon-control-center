/* cinnamon-region-panel-xkb.c
 * Copyright (C) 2003-2007 Sergey V. Udaltsov
 *
 * Written by: Sergey V. Udaltsov <svu@gnome.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street - Suite 500, Boston, MA
 * 02110-1335, USA.
 */

#include <config.h>

#include <string.h>
#include <gdk/gdkx.h>
#include <glib/gi18n.h>

#include "cinnamon-region-panel-xkb.h"

#include <libgnomekbd/gkbd-desktop-config.h>

#define CINNAMON_DESKTOP_INTERFACE_SCHEMA "org.cinnamon.desktop.interface"
#define CINNAMON_DESKTOP_FLAG_KEY "keyboard-layout-show-flags"
#define CINNAMON_DESKTOP_VARIANT_KEY "keyboard-layout-prefer-variant-names"
#define CINNAMON_DESKTOP_UPPER_CASE_KEY "keyboard-layout-use-upper"

#define GKBD_CONFIG_KEY_LOAD_EXTRA_ITEMS "load-extra-items"

// These were removed from the API of libgnomekbd in version 3.28
const gchar GKBD_DESKTOP_CONFIG_KEY_DEFAULT_GROUP[] = "default-group";
const gchar GKBD_DESKTOP_CONFIG_KEY_GROUP_PER_WINDOW[] = "group-per-window";
const gchar GKBD_KEYBOARD_CONFIG_KEY_LAYOUTS[] = "layouts";
const gchar GKBD_KEYBOARD_CONFIG_KEY_OPTIONS[] = "options";

XklEngine *engine;
XklConfigRegistry *config_registry;

GkbdKeyboardConfig initial_config;
GkbdDesktopConfig desktop_config;

GSettings *xkb_keyboard_settings;
GSettings *xkb_desktop_settings;
GSettings *cinnamon_desktop_settings;

char *
xci_desc_to_utf8 (const XklConfigItem * ci)
{
	gchar *dd = g_strdup (ci->description);
	gchar *sd = g_strstrip (dd);
	gchar *rv = g_strdup (sd[0] == 0 ? ci->name : sd);
	g_free (dd);
	return rv;
}

static void
reset_to_defaults (GtkWidget * button, GtkBuilder * dialog)
{
	GkbdKeyboardConfig empty_kbd_config;

	gkbd_keyboard_config_init (&empty_kbd_config, engine);
	gkbd_keyboard_config_save (&empty_kbd_config);
	gkbd_keyboard_config_term (&empty_kbd_config);

	g_settings_reset (xkb_desktop_settings,
			  GKBD_DESKTOP_CONFIG_KEY_DEFAULT_GROUP);

	/* all the rest is g-s-d's business */
}

static void
chk_new_windows_inherit_layout_toggled (GtkWidget *
					chk_new_windows_inherit_layout,
					GtkBuilder * dialog)
{
	xkb_save_default_group (gtk_toggle_button_get_active
				(GTK_TOGGLE_BUTTON
				 (chk_new_windows_inherit_layout)) ? -1 :
				0);
}

static void
on_load_extra_items_toggled (GtkWidget *widget,
                             gpointer   user_data)
{
    g_settings_set_boolean (xkb_desktop_settings,
                            GKBD_CONFIG_KEY_LOAD_EXTRA_ITEMS,
                            gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)));

    gkbd_desktop_config_load (&desktop_config);

    xkl_config_registry_load (config_registry,
                              desktop_config.load_extra_items);
}

void
setup_xkb_tabs (GtkBuilder * dialog)
{
	GtkWidget *widget;
	GtkStyleContext *context;
	GtkWidget *chk_new_windows_inherit_layout;

	chk_new_windows_inherit_layout = WID ("chk_new_windows_inherit_layout");

	xkb_desktop_settings = g_settings_new (GKBD_DESKTOP_SCHEMA);
	xkb_keyboard_settings = g_settings_new (GKBD_KEYBOARD_SCHEMA);

    cinnamon_desktop_settings = g_settings_new (CINNAMON_DESKTOP_INTERFACE_SCHEMA);

	engine =
	    xkl_engine_get_instance (GDK_DISPLAY_XDISPLAY
				     (gdk_display_get_default ()));
	config_registry = xkl_config_registry_get_instance (engine);

	gkbd_desktop_config_init (&desktop_config, engine);
	gkbd_desktop_config_load (&desktop_config);

	xkl_config_registry_load (config_registry,
				  desktop_config.load_extra_items);

	gkbd_keyboard_config_init (&initial_config, engine);
	gkbd_keyboard_config_load_from_x_initial (&initial_config, NULL);

	/* Set initial state */
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (WID ("chk_separate_group_per_window")),
				      g_settings_get_boolean (xkb_desktop_settings,
							      GKBD_DESKTOP_CONFIG_KEY_GROUP_PER_WINDOW));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (chk_new_windows_inherit_layout),
				      xkb_get_default_group () < 0);

	g_settings_bind (xkb_desktop_settings,
			 GKBD_DESKTOP_CONFIG_KEY_GROUP_PER_WINDOW,
			 WID ("chk_separate_group_per_window"), "active",
			 G_SETTINGS_BIND_DEFAULT);
	g_settings_bind (xkb_desktop_settings,
			 GKBD_DESKTOP_CONFIG_KEY_GROUP_PER_WINDOW,
			 WID ("chk_new_windows_inherit_layout"), "sensitive",
			 G_SETTINGS_BIND_DEFAULT);
	g_settings_bind (xkb_desktop_settings,
			 GKBD_DESKTOP_CONFIG_KEY_GROUP_PER_WINDOW,
			 WID ("chk_new_windows_default_layout"), "sensitive",
			 G_SETTINGS_BIND_DEFAULT);

    g_settings_bind (cinnamon_desktop_settings,
                     CINNAMON_DESKTOP_FLAG_KEY,
                     WID ("country-flag-check"),
                     "active",
                     G_SETTINGS_BIND_DEFAULT);

    g_settings_bind (cinnamon_desktop_settings,
                     CINNAMON_DESKTOP_VARIANT_KEY,
                     WID ("prefer-variant-check"),
                     "active",
                     G_SETTINGS_BIND_DEFAULT);

    g_settings_bind (cinnamon_desktop_settings,
                     CINNAMON_DESKTOP_UPPER_CASE_KEY,
                     WID ("use-caps-check"),
                     "active",
                     G_SETTINGS_BIND_DEFAULT);

	xkb_layouts_prepare_selected_tree (dialog);
	xkb_layouts_fill_selected_tree (dialog);

	xkb_layouts_register_buttons_handlers (dialog);
	g_signal_connect (G_OBJECT (WID ("xkb_reset_to_defaults")),
			  "clicked", G_CALLBACK (reset_to_defaults),
			  dialog);

	g_signal_connect (G_OBJECT (chk_new_windows_inherit_layout),
			  "toggled",
			  G_CALLBACK
			  (chk_new_windows_inherit_layout_toggled),
			  dialog);

	g_signal_connect_swapped (G_OBJECT (WID ("xkb_layout_options")),
				  "clicked",
				  G_CALLBACK (xkb_options_popup_dialog),
				  dialog);

    widget = WID ("load_extras_checkbox");

    g_settings_bind (xkb_desktop_settings,
                     GKBD_CONFIG_KEY_LOAD_EXTRA_ITEMS,
                     widget, "active",
                     G_SETTINGS_BIND_GET);

    g_signal_connect (widget,
                      "toggled",
                      G_CALLBACK (on_load_extra_items_toggled),
                      dialog);

	xkb_layouts_register_conf_listener (dialog);
	xkb_options_register_conf_listener (dialog);

	enable_disable_restoring (dialog);

	/* Setup junction between toolbar and treeview */
	widget = WID ("xkb_layouts_swindow");
	context = gtk_widget_get_style_context (widget);
	gtk_style_context_set_junction_sides (context, GTK_JUNCTION_BOTTOM);
	widget = WID ("layouts-toolbar");
	context = gtk_widget_get_style_context (widget);
	gtk_style_context_set_junction_sides (context, GTK_JUNCTION_TOP);
}

void
enable_disable_restoring (GtkBuilder * dialog)
{
	GkbdKeyboardConfig gswic;
	gboolean enable;

	gkbd_keyboard_config_init (&gswic, engine);
	gkbd_keyboard_config_load (&gswic, NULL);

	enable = !gkbd_keyboard_config_equals (&gswic, &initial_config);

	gkbd_keyboard_config_term (&gswic);
	gtk_widget_set_sensitive (WID ("xkb_reset_to_defaults"), enable);
}
