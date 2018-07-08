/* cinnamon-region-panel-xkbpv.c
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

#include <libgnomekbd/gkbd-keyboard-drawing.h>

#include "cinnamon-region-panel-xkb.h"

#ifdef HAVE_X11_EXTENSIONS_XKB_H
#include "X11/XKBlib.h"
/**
 * BAD STYLE: Taken from xklavier_private_xkb.h
 * Any ideas on architectural improvements are WELCOME
 */
extern gboolean xkl_xkb_config_native_prepare (XklEngine * engine,
					       const XklConfigRec * data,
					       XkbComponentNamesPtr
					       component_names);

extern void xkl_xkb_config_native_cleanup (XklEngine * engine,
					   XkbComponentNamesPtr
					   component_names);

/* */
#endif

static GkbdKeyboardDrawingGroupLevel groupsLevels[] =
    { {0, 1}, {0, 3}, {0, 0}, {0, 2} };
static GkbdKeyboardDrawingGroupLevel *pGroupsLevels[] = {
	groupsLevels, groupsLevels + 1, groupsLevels + 2, groupsLevels + 3
};

GtkWidget *
xkb_layout_preview_create_widget (GtkBuilder * chooserDialog)
{
	GtkWidget *kbdraw = gkbd_keyboard_drawing_new ();

	gkbd_keyboard_drawing_set_groups_levels (GKBD_KEYBOARD_DRAWING
						 (kbdraw), pGroupsLevels);
	return kbdraw;
}

void
xkb_layout_preview_set_drawing_layout (GtkWidget * kbdraw,
				       const gchar * id)
{
#ifdef HAVE_X11_EXTENSIONS_XKB_H
	if (kbdraw != NULL) {
		if (id != NULL) {
			XklConfigRec *data;
			char **p, *layout, *variant;
			XkbComponentNamesRec component_names;

			data = xkl_config_rec_new ();
			if (xkl_config_rec_get_from_server (data, engine)) {
				if ((p = data->layouts) != NULL)
					g_strfreev (data->layouts);

				if ((p = data->variants) != NULL)
					g_strfreev (data->variants);

				data->layouts = g_new0 (char *, 2);
				data->variants = g_new0 (char *, 2);
				if (gkbd_keyboard_config_split_items
				    (id, &layout, &variant)
				    && variant != NULL) {
					data->layouts[0] =
					    (layout ==
					     NULL) ? NULL :
					    g_strdup (layout);
					data->variants[0] =
					    (variant ==
					     NULL) ? NULL :
					    g_strdup (variant);
				} else {
					data->layouts[0] =
					    (id ==
					     NULL) ? NULL : g_strdup (id);
					data->variants[0] = NULL;
				}

				if (xkl_xkb_config_native_prepare
				    (engine, data, &component_names)) {
					gkbd_keyboard_drawing_set_keyboard
					    (GKBD_KEYBOARD_DRAWING
					     (kbdraw), &component_names);

					xkl_xkb_config_native_cleanup
					    (engine, &component_names);
				}
			}
			g_object_unref (G_OBJECT (data));
		} else
			gkbd_keyboard_drawing_set_keyboard
			    (GKBD_KEYBOARD_DRAWING (kbdraw), NULL);

	}
#endif
}
